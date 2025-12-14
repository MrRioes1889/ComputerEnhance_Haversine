#include "shm_json.h"
#include <string.h>
#include <stdio.h>

#ifndef SHM_JSON_CUSTOM_ALLOC
#include <malloc.h>
#define SHM_JSON_ALLOC(size) malloc(size)
#define SHM_JSON_REALLOC(mem, new_size) realloc(mem, new_size)
#define SHM_JSON_FREE(mem) free(mem)
#endif

#define INVALID_OFFSET 0xFFFFFFFF

enum 
{
    JsonNodeType_None = 0,
    JsonNodeType_Object,
    JsonNodeType_Array,
    JsonNodeType_Bool,
    JsonNodeType_Integer,
    JsonNodeType_Float,
    JsonNodeType_String,
    JsonNodeType_Null
};
typedef uint8 JsonNodeTypeValue;

typedef struct
{
    JsonNodeTypeValue node_type;
    uint16 node_depth;
    uint32 key_string_offset;
    SHM_JsonNodeId next_node_id;
    union 
    {
        bool8 bool_value;
        int64 int_value;
        float64 float_value;
        uint32 string_value_offset;
        uint32 child_count;
    };
}
JsonNode;

typedef struct
{
    SHM_JsonNodeId index;
    SHM_JsonNodeId last_child_index;
}
JsonParentRef;

typedef struct 
{
    uint32 capacity;
    uint32 count;
    JsonParentRef* parents;
}
JsonParentStack;

bool8 _json_parent_stack_init(uint32 count, JsonParentStack* out_stack)
{
    debug_assert(!out_stack->parents);
    if(!count)
        return false;
    
    out_stack->capacity = count;
    out_stack->parents = SHM_JSON_ALLOC(out_stack->capacity * sizeof(out_stack->parents[0]));
    memset(out_stack->parents, 0, out_stack->capacity * sizeof(out_stack->parents[0]));
    out_stack->count = 0;
    return true;
}

void _json_parent_stack_destroy(JsonParentStack* stack)
{
    SHM_JSON_FREE(stack->parents);
    stack->parents = 0;
    stack->capacity = 0;
    stack->count = 0;
}

static inline void _json_parent_stack_resize(JsonParentStack* stack, uint32 reserve_count)
{
    while (stack->capacity < (reserve_count + 1))
        stack->capacity *= 2;

    stack->parents = SHM_JSON_REALLOC(stack->parents, stack->capacity * sizeof(stack->parents[0]));
}

static inline void _json_parent_stack_push(JsonParentStack* stack, JsonParentRef parent)
{
    if (stack->count + 1 > stack->capacity)
        _json_parent_stack_resize(stack, stack->count + 1);

    stack->parents[stack->count++] = parent;
}

static inline void _json_parent_stack_pop(JsonParentStack* stack)
{
    stack->count--;
}

static inline JsonParentRef* _json_parent_stack_peek(JsonParentStack* stack)
{
    return stack->count ? &stack->parents[stack->count-1] : 0;
}

static inline bool8 _is_whitespace(char c)
{
    return (c == ' ' || c == '\n' || c == '\r' || c == '\t');
}

static inline uint32 _add_to_string_buffer(SHM_JsonData* json_data, const char* s, uint32 length)
{
    uint32 needed_size = json_data->string_buffer_total_length + length + 1;
    if (json_data->string_buffer_size < needed_size)
    {
        do
        {
            json_data->string_buffer_size *= 2;
        } 
        while (json_data->string_buffer_size < needed_size);

        json_data->string_buffer = SHM_JSON_REALLOC(json_data->string_buffer, json_data->string_buffer_size);
    }

    uint32 offset = json_data->string_buffer_total_length;
    memcpy(&json_data->string_buffer[offset], s, length);
    json_data->string_buffer_total_length += length + 1;
    json_data->string_buffer[json_data->string_buffer_total_length-1] = 0;
    return offset;
}

static inline uint32 _add_to_node_buffer(SHM_JsonData* json_data, JsonNode node)
{
    if (json_data->node_count + 1 > json_data->node_buffer_size)
    {
        json_data->node_buffer_size *= 2;
        json_data->node_buffer = SHM_JSON_REALLOC(json_data->node_buffer, (json_data->node_buffer_size * sizeof(JsonNode)));
    }
    JsonNode* node_buffer = json_data->node_buffer;

    uint32 index = json_data->node_count;
    node_buffer[index] = node;
    json_data->node_count++;
    return index;
}

static inline uint32 _parse_int64(const char* s, int64* out_value)
{
    uint32 pre_length = 0;
    int32 sign_mod = 1;
    if (*s == '-')
    {
        sign_mod = -1;
        s++;
        pre_length++;
    }

    uint32 num_length = 0;
    uint64 abs_value = 0; 
    while (*s >= '0' && *s <= '9')
    {
        abs_value = (abs_value * 10) + ((*s) - '0');
        s++;
        num_length++;
    }

    if (!num_length)
        return 0;

    *out_value = (abs_value * sign_mod);
    return pre_length + num_length;
}

static inline uint32 _parse_float64(const char* s, float64* out_value)
{
    uint32 pre_length = 0;
    int32 sign_mod = 1;
    if (*s == '-')
    {
        sign_mod = -1;
        s++;
        pre_length++;
    }

    uint32 num_length = 0;
    float64 abs_value = 0.0; 
    while (*s >= '0' && *s <= '9')
    {
        abs_value = (abs_value * 10) + ((*s) - '0');
        s++;
        num_length++;
    }

    if (!num_length)
        return 0;

    num_length += pre_length;
    if (*s != '.')
    {
        *out_value = (abs_value * sign_mod);
        return num_length;
    }

    s++;
    num_length++;
    float64 mult = 0.1;
    while (*s >= '0' && *s <= '9')
    {
        abs_value += ((*s) - '0') * mult;
        mult *= 0.1;
        num_length++;
        s++;
    }

    *out_value = (abs_value * sign_mod);
    return num_length;
}

bool8 shm_json_parse_text(const char* text, uint32 estimated_node_count, SHM_JsonData* out_json)
{
    estimated_node_count = MAX(estimated_node_count, 1);

    SHM_JsonData json_data = {0};
    json_data.node_buffer_size = estimated_node_count;
    json_data.node_buffer = SHM_JSON_ALLOC(json_data.node_buffer_size * sizeof(JsonNode));
    json_data.node_count = 0;

    json_data.string_buffer_size = estimated_node_count;
    json_data.string_buffer = SHM_JSON_ALLOC(json_data.string_buffer_size);
    json_data.string_buffer_total_length = 0;

    JsonParentStack parent_stack = {0};
    _json_parent_stack_init(16, &parent_stack);

    uint32 node_depth = 0;
    uint32 last_key_string_offset = _add_to_string_buffer(&json_data, "head", 4);
    bool8 parsing_value = true;
    JsonNodeTypeValue parent_type = JsonNodeType_None;

    const char* read_ptr = text;
    while (*read_ptr)
    {
        char c = *read_ptr;
        if (_is_whitespace(c))
        {
            read_ptr++;
            continue;
        }

        switch (c)
        {
            case '{':
            {
                if (!parsing_value)
                    goto parse_error;

                JsonNode node = {.node_type = JsonNodeType_Object, .key_string_offset = last_key_string_offset, .node_depth = node_depth, .next_node_id = SHM_JSON_INVALID_ID};
                uint32 node_id = _add_to_node_buffer(&json_data, node);
                JsonNode* node_buffer = json_data.node_buffer;
                if (parent_stack.count)
                {
                    JsonParentRef* parent_ref = _json_parent_stack_peek(&parent_stack);
                    if (parent_ref->last_child_index != SHM_JSON_INVALID_ID) node_buffer[parent_ref->last_child_index].next_node_id = node_id;
                    parent_ref->last_child_index = node_id;
                    node_buffer[parent_ref->index].child_count++;
                }
                _json_parent_stack_push(&parent_stack, (JsonParentRef){.index = node_id, .last_child_index = SHM_JSON_INVALID_ID});
                parent_type = node.node_type;
                last_key_string_offset = INVALID_OFFSET;
                node_depth++;
                parsing_value = false;
                break;
            }
            case '}':
            {
                if (!parent_stack.count || parent_type != JsonNodeType_Object)
                    goto parse_error;

                _json_parent_stack_pop(&parent_stack);
                node_depth--;
                if (parent_stack.count)
                {
                    JsonParentRef* parent_ref = _json_parent_stack_peek(&parent_stack);
                    JsonNode* node_buffer = json_data.node_buffer;
                    parent_type = node_buffer[parent_ref->index].node_type;
                }
                break;
            }
            case '[':
            {
                if (!parsing_value)
                    goto parse_error;

                JsonNode node = {.node_type = JsonNodeType_Array, .key_string_offset = last_key_string_offset, .node_depth = node_depth, .next_node_id = SHM_JSON_INVALID_ID};
                uint32 node_id = _add_to_node_buffer(&json_data, node);
                JsonNode* node_buffer = json_data.node_buffer;
                if (parent_stack.count)
                {
                    JsonParentRef* parent_ref = _json_parent_stack_peek(&parent_stack);
                    if (parent_ref->last_child_index != SHM_JSON_INVALID_ID) node_buffer[parent_ref->last_child_index].next_node_id = node_id;
                    parent_ref->last_child_index = node_id;
                    node_buffer[parent_ref->index].child_count++;
                }
                _json_parent_stack_push(&parent_stack, (JsonParentRef){.index = node_id, .last_child_index = SHM_JSON_INVALID_ID});
                parent_type = node.node_type;
                last_key_string_offset = INVALID_OFFSET;
                node_depth++;
                parsing_value = true;
                break;
            }
            case ']':
            {
                if (!parent_stack.count || parent_type != JsonNodeType_Array)
                    goto parse_error;

                _json_parent_stack_pop(&parent_stack);
                node_depth--;
                if (parent_stack.count)
                {
                    JsonParentRef* parent_ref = _json_parent_stack_peek(&parent_stack);
                    JsonNode* node_buffer = json_data.node_buffer;
                    parent_type = node_buffer[parent_ref->index].node_type;
                }
                break;
            }
            case '"':
            {
                if (!parent_stack.count)
                    goto parse_error;

                uint32 s_length = 0;
                read_ptr++;
                const char* s_start = read_ptr;
                for(; (*read_ptr != '"' && read_ptr[-1] != '\\') && *read_ptr; read_ptr++, s_length++);
                if (!*read_ptr)
                    goto parse_error;

                uint32 string_offset = _add_to_string_buffer(&json_data, s_start, s_length);
                if (parent_type == JsonNodeType_Object && last_key_string_offset == INVALID_OFFSET)
                {
                    last_key_string_offset = string_offset;
                    break;
                }

                if (!parsing_value)
                    goto parse_error;

                JsonNode node = {.node_type = JsonNodeType_String, .key_string_offset = last_key_string_offset, .node_depth = node_depth, .next_node_id = SHM_JSON_INVALID_ID, .string_value_offset = string_offset};
                uint32 node_id = _add_to_node_buffer(&json_data, node);
                JsonNode* node_buffer = json_data.node_buffer;
                JsonParentRef* parent_ref = _json_parent_stack_peek(&parent_stack);
                if (parent_ref->last_child_index != SHM_JSON_INVALID_ID) node_buffer[parent_ref->last_child_index].next_node_id = node_id;
                parent_ref->last_child_index = node_id;
                node_buffer[parent_ref->index].child_count++;
                last_key_string_offset = INVALID_OFFSET;
                parsing_value = false;
                break;
            }
            case '-':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                if (!parent_stack.count || !parsing_value)
                    goto parse_error;

                bool8 is_float = false;
                uint32 read_i = 0;
                if (*read_ptr == '-')
                    read_i++;
                for (; ((read_ptr[read_i] >= '0' && read_ptr[read_i] <= '9') || read_ptr[read_i] == '.'); read_i++)
                {
                    if (read_ptr[read_i] == '.')
                    {
                        is_float = true;
                        break;
                    }
                }

                JsonNode node = {0};
                uint32 s_length = 0;
                if (is_float)
                {
                    node = (JsonNode){.node_type = JsonNodeType_Float, .key_string_offset = last_key_string_offset, .node_depth = node_depth, .next_node_id = SHM_JSON_INVALID_ID};
                    s_length = _parse_float64(read_ptr, &node.float_value);
                }
                else
                {
                    node = (JsonNode){.node_type = JsonNodeType_Integer, .key_string_offset = last_key_string_offset, .node_depth = node_depth, .next_node_id = SHM_JSON_INVALID_ID};
                    s_length = _parse_int64(read_ptr, &node.int_value);
                }
                if (!s_length)
                    goto parse_error;
                read_ptr += (s_length - 1);

                uint32 node_id = _add_to_node_buffer(&json_data, node);
                JsonNode* node_buffer = json_data.node_buffer;
                JsonParentRef* parent_ref = _json_parent_stack_peek(&parent_stack);
                if (parent_ref->last_child_index != SHM_JSON_INVALID_ID) node_buffer[parent_ref->last_child_index].next_node_id = node_id;
                parent_ref->last_child_index = node_id;
                node_buffer[parent_ref->index].child_count++;
                last_key_string_offset = INVALID_OFFSET;
                parsing_value = false;
                break;
            }
            case 't':
            {
                if (!parent_stack.count || !parsing_value)
                    goto parse_error;
                
                if (strncmp(read_ptr, "true", 4) == 0)
                    read_ptr += 3;
                else
                    goto parse_error;

                JsonNode node = {.node_type = JsonNodeType_Bool, .key_string_offset = last_key_string_offset, .node_depth = node_depth, .next_node_id = SHM_JSON_INVALID_ID, .bool_value = true};

                uint32 node_id = _add_to_node_buffer(&json_data, node);
                JsonNode* node_buffer = json_data.node_buffer;
                JsonParentRef* parent_ref = _json_parent_stack_peek(&parent_stack);
                if (parent_ref->last_child_index != SHM_JSON_INVALID_ID) node_buffer[parent_ref->last_child_index].next_node_id = node_id;
                parent_ref->last_child_index = node_id;
                node_buffer[parent_ref->index].child_count++;
                last_key_string_offset = INVALID_OFFSET;
                parsing_value = false;
                break;
            }
            case 'f':
            {
                if (!parent_stack.count || !parsing_value)
                    goto parse_error;
                
                if (strncmp(read_ptr, "false", 5) == 0)
                    read_ptr += 4;
                else
                    goto parse_error;

                JsonNode node = {.node_type = JsonNodeType_Bool, .key_string_offset = last_key_string_offset, .node_depth = node_depth, .next_node_id = SHM_JSON_INVALID_ID, .bool_value = false};

                uint32 node_id = _add_to_node_buffer(&json_data, node);
                JsonNode* node_buffer = json_data.node_buffer;
                JsonParentRef* parent_ref = _json_parent_stack_peek(&parent_stack);
                if (parent_ref->last_child_index != SHM_JSON_INVALID_ID) node_buffer[parent_ref->last_child_index].next_node_id = node_id;
                parent_ref->last_child_index = node_id;
                node_buffer[parent_ref->index].child_count++;
                last_key_string_offset = INVALID_OFFSET;
                parsing_value = false;
                break;
            }
            case 'n':
            {
                if (!parent_stack.count || !parsing_value)
                    goto parse_error;
                
                if (strncmp(read_ptr, "null", 4) == 0)
                    read_ptr += 3;
                else
                    goto parse_error;

                JsonNode node = {.node_type = JsonNodeType_Null, .key_string_offset = last_key_string_offset, .node_depth = node_depth, .next_node_id = SHM_JSON_INVALID_ID};

                uint32 node_id = _add_to_node_buffer(&json_data, node);
                JsonNode* node_buffer = json_data.node_buffer;
                JsonParentRef* parent_ref = _json_parent_stack_peek(&parent_stack);
                if (parent_ref->last_child_index != SHM_JSON_INVALID_ID) node_buffer[parent_ref->last_child_index].next_node_id = node_id;
                parent_ref->last_child_index = node_id;
                node_buffer[parent_ref->index].child_count++;
                last_key_string_offset = INVALID_OFFSET;
                parsing_value = false;
                break;
            }
            case ':':
            {
                if (parent_type != JsonNodeType_Object || last_key_string_offset == INVALID_OFFSET || parsing_value)
                    goto parse_error;

                parsing_value = true;
                break;
            }
            case ',':
            {
                if (parent_type == JsonNodeType_Array)
                    parsing_value = true;

                break;
            }
            default:
            {
                goto parse_error;
            }
        }
        
        read_ptr++;
    }

    if (parent_stack.count > 0)
        goto parse_error;

    _json_parent_stack_destroy(&parent_stack);
    *out_json = json_data;
    return true;

parse_error:
    _json_parent_stack_destroy(&parent_stack);
    shm_json_data_destroy(&json_data);
    *out_json = json_data;
    return false;
}

void shm_json_print_data(const SHM_JsonData* data)
{
    const JsonNode* node_buffer = (const JsonNode*)data->node_buffer;
    for (uint32 i = 0; i < data->node_count; i++)
    {
        JsonNode node = node_buffer[i];
        const char* key = node.key_string_offset == INVALID_OFFSET ? "" : &data->string_buffer[node.key_string_offset];
        switch (node.node_type)
        {
        case JsonNodeType_Object:
        {
            printf_s("{node_id = %u, type = Object, depth = %hu, next_id = %u, key = '%s', child_count = %u}\n",
                 i, node.node_depth, node.next_node_id, key, node.child_count);
            break;
        }
        case JsonNodeType_Array:
        {
            printf_s("{node_id = %u, type = Array, depth = %hu, next_id = %u, key = '%s', child_count = %u}\n",
                 i, node.node_depth, node.next_node_id, key, node.child_count);
            break;
        }
        case JsonNodeType_String:
        {
            printf_s("{node_id = %u, type = String, depth = %hu, next_id = %u, key = '%s', string_value = %u}\n",
                 i, node.node_depth, node.next_node_id, key, &data->string_buffer[node.string_value_offset]);
            break;
        }
        case JsonNodeType_Float:
        {
            printf_s("{node_id = %u, type = Float, depth = %hu, next_id = %u, key = '%s', value = %lf}\n",
                 i, node.node_depth, node.next_node_id, key, node.float_value);
            break;
        }
        case JsonNodeType_Integer:
        {
            printf_s("{node_id = %u, type = Integer, depth = %hu, next_id = %u, key = '%s', value = %li}\n",
                 i, node.node_depth, node.next_node_id, key, node.int_value);
            break;
        }
        case JsonNodeType_Bool:
        {
            printf_s("{node_id = %u, type = Bool, depth = %hu, next_id = %u, key = '%s', value = %s}\n",
                 i, node.node_depth, node.next_node_id, key, node.bool_value ? "true" : "false");
            break;
        }
        case JsonNodeType_Null:
        {
            printf_s("{node_id = %u, type = Null, depth = %hu, next_id = %u, key = '%s'}\n",
                 i, node.node_depth, node.next_node_id, key);
            break;
        }
        }
    }
}

void shm_json_data_destroy(SHM_JsonData* data)
{
    SHM_JSON_FREE(data->node_buffer);
    SHM_JSON_FREE(data->string_buffer);
    *data = (SHM_JsonData){0};
}

SHM_JsonNodeId shm_json_get_child_id_by_key(const SHM_JsonData* data, SHM_JsonNodeId parent_id, const char* key)
{
    if (parent_id > data->node_count - 1)
        return SHM_JSON_INVALID_ID;

    JsonNode* node_buffer = data->node_buffer;
    JsonNode parent = node_buffer[parent_id];

    if ((parent.node_type != JsonNodeType_Object && parent.node_type != JsonNodeType_Array) || parent.child_count == 0)
        return SHM_JSON_INVALID_ID;

    SHM_JsonNodeId child_id = parent_id + 1;
    while (child_id != SHM_JSON_INVALID_ID)
    {
        JsonNode child = node_buffer[child_id];
        if (child.key_string_offset != INVALID_OFFSET && strcmp(key, &data->string_buffer[child.key_string_offset]) == 0)
            return child_id;

        child_id = child.next_node_id;
    }

    return SHM_JSON_INVALID_ID;
}

SHM_JsonNodeId shm_json_get_child_id_by_index(const SHM_JsonData* data, SHM_JsonNodeId parent_id, uint32 index)
{
    if (parent_id > data->node_count - 1)
        return SHM_JSON_INVALID_ID;

    JsonNode* node_buffer = data->node_buffer;
    JsonNode parent = node_buffer[parent_id];

    if ((parent.node_type != JsonNodeType_Object && parent.node_type != JsonNodeType_Array) || parent.child_count <= index)
        return SHM_JSON_INVALID_ID;

    SHM_JsonNodeId child_id = parent_id + 1;
    for (uint32 i = 0; i < index; i++)
        child_id = node_buffer[child_id].next_node_id;

    return child_id;
}

SHM_JsonNodeId shm_json_get_first_child_id(const SHM_JsonData* data, SHM_JsonNodeId parent_id)
{
    if (parent_id > data->node_count - 1)
        return SHM_JSON_INVALID_ID;

    JsonNode* node_buffer = data->node_buffer;
    JsonNode parent = node_buffer[parent_id];

    if ((parent.node_type != JsonNodeType_Object && parent.node_type != JsonNodeType_Array) || parent.child_count == 0)
        return SHM_JSON_INVALID_ID;

    return parent_id + 1;
}

SHM_JsonNodeId shm_json_get_next_child_id(const SHM_JsonData* data, SHM_JsonNodeId sibling_id)
{
    if (sibling_id > data->node_count - 1)
        return SHM_JSON_INVALID_ID;

    JsonNode* node_buffer = data->node_buffer;
    return node_buffer[sibling_id].next_node_id;
}

uint32 shm_json_get_child_count(const SHM_JsonData* data, SHM_JsonNodeId node_id)
{
    if (node_id > data->node_count - 1)
        return 0;

    JsonNode* node_buffer = data->node_buffer;
    JsonNode node = node_buffer[node_id];

    if (node.node_type != JsonNodeType_Object && node.node_type != JsonNodeType_Array)
        return 0;

    return node.child_count;
}

bool8 shm_json_get_float_value(const SHM_JsonData* data, SHM_JsonNodeId node_id, float64* out_value)
{
    if (node_id > data->node_count - 1)
        return false;

    JsonNode* node_buffer = data->node_buffer;
    JsonNode node = node_buffer[node_id];

    if (node.node_type != JsonNodeType_Float)
        return false;

    *out_value = node.float_value;
    return true;
}

bool8 shm_json_get_int_value(const SHM_JsonData* data, SHM_JsonNodeId node_id, int64* out_value)
{
    if (node_id > data->node_count - 1)
        return false;

    JsonNode* node_buffer = data->node_buffer;
    JsonNode node = node_buffer[node_id];

    if (node.node_type != JsonNodeType_Integer)
        return false;

    *out_value = node.int_value;
    return true;
}

bool8 shm_json_get_string_value(const SHM_JsonData* data, SHM_JsonNodeId node_id, const char** out_value)
{
    if (node_id > data->node_count - 1)
        return false;

    JsonNode* node_buffer = data->node_buffer;
    JsonNode node = node_buffer[node_id];

    if (node.node_type != JsonNodeType_String)
        return false;

    *out_value = &data->string_buffer[node.string_value_offset];
    return true;
}

bool8 shm_json_get_bool_value(const SHM_JsonData* data, SHM_JsonNodeId node_id, bool8* out_value)
{
    if (node_id > data->node_count - 1)
        return false;

    JsonNode* node_buffer = data->node_buffer;
    JsonNode node = node_buffer[node_id];

    if (node.node_type != JsonNodeType_Bool)
        return false;

    *out_value = node.bool_value;
    return true;
}