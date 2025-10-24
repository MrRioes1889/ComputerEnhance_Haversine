#include "shm_string.h"

uint32 cstring_copy(char* dst_buf, uint32 dst_buf_size, const char* src_buf)
{
    uint32 copy_length = 0;
    uint32 max_copy_length = dst_buf_size - 1;
    while(copy_length < max_copy_length && (*dst_buf++ = *src_buf++))
        copy_length++;

    *dst_buf = 0;
    return copy_length;
}

uint32 cstring_copy_n(char* dst_buf, uint32 dst_buf_size, const char* src_buf, uint32 length)
{
    uint32 copy_length = 0;
    uint32 max_copy_length = dst_buf_size - 1 < length ? dst_buf_size - 1 : length;
    while(copy_length < max_copy_length && (*dst_buf++ = *src_buf++))
        copy_length++;

    *dst_buf = 0;
    return copy_length;
}

uint32 cstring_length(const char* s)
{
    uint32 length = 0;
    while (*s++)
        length++;

    return length;
}

int32 cstring_first_index_of(const char* s, char c)
{
    for (int32 i = 0; s[i]; i++)
    {
        if (s[i] == c)
            return i;
    }

    return -1;
}

int32 cstring_last_index_of(const char* s, char c)
{
    int32 index = -1;
    for (int32 i = 0; s[i]; i++)
    {
        if (s[i] == c)
            index = i;
    }

    return index;
}