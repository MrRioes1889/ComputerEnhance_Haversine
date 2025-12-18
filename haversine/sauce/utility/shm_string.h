#pragma once
#include "shm_utils_common_defines.h"

typedef struct
{
    char* buffer;
    uint64 buffer_size;
    uint64 length;
}
SHM_String;

uint32 shm_cstring_copy(char* dst_buf, uint32 dst_buf_size, const char* src_buf);
uint32 shm_cstring_copy_n(char* dst_buf, uint32 dst_buf_size, const char* src_buf, uint32 length);

uint32 shm_cstring_length(const char* s);
int32 shm_cstring_first_index_of(const char* s, char c);
int32 shm_cstring_last_index_of(const char* s, char c);

bool8 shm_cstring_equal(const char* a, const char* b);

bool8 shm_string_init(uint64 length, SHM_String* out_string);
void shm_string_destroy(SHM_String* string);
void shm_string_reserve(SHM_String* string, uint64 reserve_length);

void shm_string_copy_c(SHM_String* dest, const char* src);
void shm_string_copy_s(SHM_String* dest, const SHM_String* src);
void shm_string_append_c(SHM_String* dest, const char* appendage);
void shm_string_append_s(SHM_String* dest, const SHM_String* appendage);