#include "shm_string.h"
#include <string.h>

#ifndef SHM_STRING_CUSTOM_ALLOC
#include <malloc.h>
#define SHM_STRING_ALLOC(size) malloc(size)
#define SHM_STRING_REALLOC(mem, new_size) realloc(mem, new_size)
#define SHM_STRING_FREE(mem) free(mem)
#endif

#define SHM_STRING_MIN_SIZE 8
#define SHM_STRING_RESIZE_FACTOR 1.5f

uint32 shm_cstring_copy(char* dst_buf, uint32 dst_buf_size, const char* src_buf)
{
    uint32 copy_length = 0;
    uint32 max_copy_length = dst_buf_size - 1;
    while(copy_length < max_copy_length && (*dst_buf++ = *src_buf++))
        copy_length++;

    *dst_buf = 0;
    return copy_length;
}

uint32 shm_cstring_copy_n(char* dst_buf, uint32 dst_buf_size, const char* src_buf, uint32 length)
{
    uint32 copy_length = 0;
    uint32 max_copy_length = dst_buf_size - 1 < length ? dst_buf_size - 1 : length;
    while(copy_length < max_copy_length && (*dst_buf++ = *src_buf++))
        copy_length++;

    *dst_buf = 0;
    return copy_length;
}

uint32 shm_cstring_length(const char* s)
{
    uint32 length = 0;
    while (*s++)
        length++;

    return length;
}

int32 shm_cstring_first_index_of(const char* s, char c)
{
    for (int32 i = 0; s[i]; i++)
    {
        if (s[i] == c)
            return i;
    }

    return -1;
}

int32 shm_cstring_last_index_of(const char* s, char c)
{
    int32 index = -1;
    for (int32 i = 0; s[i]; i++)
    {
        if (s[i] == c)
            index = i;
    }

    return index;
}

bool8 shm_cstring_equal(const char* a, const char* b)
{
    for (; *a; a++, b++)
    {
        if (*a != *b)
            return false;
    }

    return true;
}

bool8 shm_string_init(uint64 length, SHM_String* out_string)
{
    debug_assert(!out_string->buffer);
    if(!length)
        return false;
    
    out_string->buffer_size = MAX(SHM_STRING_MIN_SIZE, length + 1);
    out_string->buffer = SHM_STRING_ALLOC(out_string->buffer_size);
    memset(out_string->buffer, 0, out_string->buffer_size);
    out_string->length = 0;
    return true;
}

void shm_string_destroy(SHM_String* string)
{
    SHM_STRING_FREE(string->buffer);
    string->buffer = 0;
    string->buffer_size = 0;
    string->length = 0;
}

void shm_string_reserve(SHM_String* string, uint64 reserve_length)
{
    if (!string->buffer)
    {
        shm_string_init(reserve_length, string);
        return;
    }

    if (string->buffer_size > reserve_length)
        return;

    while (string->buffer_size < (reserve_length + 1))
        string->buffer_size *= SHM_STRING_RESIZE_FACTOR;
    string->buffer = SHM_STRING_REALLOC(string->buffer, string->buffer_size);
}

void shm_string_copy_c(SHM_String* dest, const char* src)
{
    uint32 src_length = shm_cstring_length(src);
    shm_string_reserve(dest, src_length);
    memcpy_s(dest->buffer, dest->buffer_size, src, src_length);
    dest->length = src_length;
    dest->buffer[dest->length] = 0;
}

void shm_string_copy_s(SHM_String* dest, const SHM_String* src)
{
    shm_string_reserve(dest, src->length);
    memcpy_s(dest->buffer, dest->buffer_size, src->buffer, src->length);
    dest->length = src->length;
    dest->buffer[dest->length] = 0;
}

void shm_string_append_c(SHM_String* dest, const char* appendage)
{
    uint32 appendage_length = shm_cstring_length(appendage);
    uint64 reserve_length = (dest->buffer ? dest->length : 0) + appendage_length;
    shm_string_reserve(dest, reserve_length);
    memcpy_s(&dest->buffer[dest->length], dest->buffer_size - dest->length, appendage, appendage_length);
    dest->length += appendage_length;
    dest->buffer[dest->length] = 0;
}

void shm_string_append_s(SHM_String* dest, const SHM_String* appendage)
{
    uint64 reserve_length = (dest->buffer ? dest->length : 0) + appendage->length;
    shm_string_reserve(dest, reserve_length);
    memcpy_s(&dest->buffer[dest->length], dest->buffer_size - dest->length, appendage->buffer, appendage->length);
    dest->length += appendage->length;
    dest->buffer[dest->length] = 0;
}