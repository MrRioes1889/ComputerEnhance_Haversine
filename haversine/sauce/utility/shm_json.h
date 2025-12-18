#pragma once
#include "shm_utils_common_defines.h"

typedef struct
{
    uint32 node_buffer_size;
    uint32 node_count;
    uint32 string_buffer_size;
    uint32 string_buffer_total_length;
    void* node_buffer;
    char* string_buffer;
}
SHM_JsonData;

typedef uint32 SHM_JsonNodeId;
#define SHM_JSON_INVALID_ID 0xFFFFFFFF

bool8 shm_json_parse_text(const char* test, uint32 estimated_node_count, SHM_JsonData* out_json);
void shm_json_print_data(const SHM_JsonData* data);
void shm_json_data_destroy(SHM_JsonData* data);

SHM_JsonNodeId shm_json_get_child_id_by_key(const SHM_JsonData* data, SHM_JsonNodeId parent_id, const char* key);
SHM_JsonNodeId shm_json_get_child_id_by_index(const SHM_JsonData* data, SHM_JsonNodeId parent_id, uint32 index);
SHM_JsonNodeId shm_json_get_first_child_id(const SHM_JsonData* data, SHM_JsonNodeId parent_id);
SHM_JsonNodeId shm_json_get_next_child_id(const SHM_JsonData* data, SHM_JsonNodeId sibling_id);
uint32 shm_json_get_child_count(const SHM_JsonData* data, SHM_JsonNodeId node_id);

bool8 shm_json_get_float_value(const SHM_JsonData* data, SHM_JsonNodeId node_id, float64* out_value);
bool8 shm_json_get_int_value(const SHM_JsonData* data, SHM_JsonNodeId node_id, int64* out_value);
bool8 shm_json_get_string_value(const SHM_JsonData* data, SHM_JsonNodeId node_id, const char** out_value);
bool8 shm_json_get_bool_value(const SHM_JsonData* data, SHM_JsonNodeId node_id, bool8* out_value);