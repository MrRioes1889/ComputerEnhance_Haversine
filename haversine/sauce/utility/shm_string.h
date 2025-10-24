#pragma once
#include "defines.h"

typedef struct
{
    char* buffer;
    uint32 buffer_size;
    uint32 length;
}
SHM_String;

uint32 cstring_copy(char* dst_buf, uint32 dst_buf_size, const char* src_buf);
uint32 cstring_copy_n(char* dst_buf, uint32 dst_buf_size, const char* src_buf, uint32 length);

uint32 cstring_length(const char* s);
int32 cstring_first_index_of(const char* s, char c);
int32 cstring_last_index_of(const char* s, char c);