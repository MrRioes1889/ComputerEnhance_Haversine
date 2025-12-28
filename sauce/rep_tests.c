#include "rep_tests.h"
#include "shm_utils/shm_repetition_tester.h"
#include "shm_utils/platform/shm_platform.h"
#include <malloc.h>
#include <stdio.h>

#pragma comment(lib, "win64_test_loops.lib")
extern void asm_mov_all_bytes(uint64 buffer_size, uint8* buffer);
extern void asm_nop_all_bytes(uint64 buffer_size, uint8* buffer);
extern void asm_inc_cmp_empty_loop(uint64 buffer_size, uint8* buffer);
extern void asm_dec_empty_loop(uint64 buffer_size, uint8* buffer);

extern void asm_read_mov_x1(uint64 buffer_size, uint8* buffer);
extern void asm_read_mov_x2(uint64 buffer_size, uint8* buffer);
extern void asm_read_mov_x3(uint64 buffer_size, uint8* buffer);
extern void asm_read_mov_x4(uint64 buffer_size, uint8* buffer);

extern void asm_wide_read_mov_4x2(uint64 buffer_size, uint8* buffer);
extern void asm_wide_read_mov_8x2(uint64 buffer_size, uint8* buffer);
extern void asm_wide_read_mov_16x2(uint64 buffer_size, uint8* buffer);
extern void asm_wide_read_mov_32x2(uint64 buffer_size, uint8* buffer);

// CONSTRAINTS: buffer_size: multiple of 128, slice_size: power of 2 && >= 128
extern void asm_cache_size_test_128(uint64 buffer_size, uint8* buffer, uint64 slice_size);
// CONSTRAINTS: buffer_size: multiple of 256, slice_size: power of 2 && >= 256
extern void asm_cache_size_test_256(uint64 buffer_size, uint8* buffer, uint64 slice_size);
// CONSTRAINTS: buffer_size: >= 256, slice_size: * 256 <= buffer_size
extern uint64 asm_cache_size_test_256_non_pow_of_2(uint64 buffer_size, uint8* buffer, uint64 read_block_256_count);

typedef struct 
{
    uint64 size;
    void* data;
}
TestBuffer;

typedef struct
{
    const char* filename;
    TestBuffer preallocated_buffer;
}
TestParams;

static inline TestBuffer _allocate_filebuffer(const char* filename)
{
    TestBuffer buffer = {0};
    uint64 filesize = shm_platform_get_filesize(filename);
    if (!filesize)
        return buffer;

    buffer.size = filesize;
    buffer.data = malloc(buffer.size);
    return buffer;
}

static inline void _free_filebuffer(TestBuffer* buffer)
{
    free(buffer->data);
    buffer->data = 0;
    buffer->size = 0;
}

static void _read_file_test_fread(SHM_RepetitionTester* tester, const char* filename, uint64 buffer_size, void* buffer)
{
    FILE* file = fopen(filename, "rb");
    if(!file)
    {
        shm_repetition_test_log_error(tester, "fread failed");
        return;
    }

    shm_repetition_test_begin_timer(tester);
    uint64 result = fread(buffer, buffer_size, 1, file);
    shm_repetition_test_end_timer(tester);
    
    if(result == 1)
    {
        shm_repetition_test_add_bytes_processed(tester, buffer_size);
    }
    else
    {
        shm_repetition_test_log_error(tester, "fread failed");
    }
    
    fclose(file);
}

static void _write_all_bytes(uint64 buffer_size, uint8* buffer)
{
    for (uint64 i = 0; i < buffer_size; i++)
    {
        buffer[i] = (uint8)i;
    }
}

typedef uint64(*FP_read_test_function)(SHM_RepetitionTester* tester, const char* filename, uint64 buffer_size, void* buffer);
typedef struct ReadTest
{
    const char* name;
    FP_read_test_function func;
}
ReadTest;

void run_file_read_tests(uint64 time_counter_frequency, const char* filename)
{
    TestBuffer buffer = _allocate_filebuffer(filename);
    if (!buffer.data)
        return;
    
    uint8* read_data = buffer.data;
    // NOTE: Preventing page faults
    for (uint64 i = 0; i < buffer.size; i++)
        read_data[i] = (uint8)i;

    ReadTest tests[] =
    {
        {.func = _read_file_test_fread, .name = "fread"}
    };
    uint32 test_count = array_count(tests);

    char test_title[256];
    bool8 running = true;
    SHM_RepetitionTester tester = {0};
    shm_repetition_tester_init(time_counter_frequency, 10.0, false, &tester);
    uint32 test_i = 0;
    while(running)
    {
        sprintf_s(test_title, array_count(test_title), "Read file test - File size: %llu Bytes - Method: %s", 
            buffer.size, tests[test_i].name);
        shm_repetition_tester_begin_test(&tester, test_title);

        while (shm_repetition_tester_next_run(&tester))
        {
            tests[test_i].func(&tester, filename, buffer.size, buffer.data);
        }

        shm_repetition_tester_print_last_test_results(&tester);
        test_i = (test_i + 1) % test_count;
    }
}

void run_cache_size_tests_pow_2(uint64 time_counter_frequency)
{
    TestBuffer buffer = {0};
    buffer.size = GIGABYTE(1);
    buffer.data = shm_platform_memory_allocate(buffer.size);
    if (!buffer.data)
        return;

    uint64 read_size = buffer.size - 64;
    uint8* read_data = buffer.data;
    // NOTE: For testing unaligned reads
    uint64 read_alignment_offset = 0;
    read_data += read_alignment_offset; //
    // NOTE: Preventing page faults
    for (uint64 i = 0; i < read_size; i++)
        read_data[i] = (uint8)i;

    char test_title[256];
    uint64 slice_size = KILOBYTE(1);
    bool8 running = true;
    SHM_RepetitionTester tester = {0};
    shm_repetition_tester_init(time_counter_frequency, 10.0, false, &tester);
    while (running)
    {
        sprintf_s(test_title, array_count(test_title), "Cache size test 8x32 movs - Slice size: %llu Bytes - Read alignment offset: %llu Bytes", 
            slice_size, read_alignment_offset);
        shm_repetition_tester_begin_test(&tester, test_title);

        while (shm_repetition_tester_next_run(&tester))
        {
            shm_repetition_test_begin_timer(&tester);
            asm_cache_size_test_256(read_size, read_data, slice_size);
            shm_repetition_test_end_timer(&tester);
            shm_repetition_test_add_bytes_processed(&tester, buffer.size);
        }

        shm_repetition_tester_print_last_test_results(&tester);
        slice_size = slice_size >= GIGABYTE(1) ? KILOBYTE(1) : slice_size << 1;
    }

    shm_platform_memory_free(buffer.data);
}

void run_cache_size_tests_non_pow_2(uint64 time_counter_frequency)
{
    TestBuffer buffer = {0};
    buffer.size = GIGABYTE(1) + 64;
    buffer.data = shm_platform_memory_allocate(buffer.size);
    if (!buffer.data)
        return;

    uint64 read_size = buffer.size - 64;
    uint8* read_data = buffer.data;
    // NOTE: For testing unaligned reads
    uint64 read_alignment_offset = 0;
    read_data += read_alignment_offset; //
    // NOTE: Preventing page faults
    for (uint64 i = 0; i < read_size; i++)
        read_data[i] = (uint8)i;

    uint64 read_block_counts[] =
    {
        2,
        4,
        16,
        3,
        303
    };
    uint32 test_count = array_count(read_block_counts);

    char test_title[256];
    bool8 running = true;
    SHM_RepetitionTester tester = {0};
    shm_repetition_tester_init(time_counter_frequency, 10.0, false, &tester);
    uint32 test_i = 0;
    while (running)
    {
        sprintf_s(test_title, array_count(test_title), "Cache size test 8x32 movs - Slice size: %llu Bytes - Read alignment offset: %llu Bytes", 
            read_block_counts[test_i] * 256, read_alignment_offset);
        shm_repetition_tester_begin_test(&tester, test_title);

        while (shm_repetition_tester_next_run(&tester))
        {
            shm_repetition_test_begin_timer(&tester);
            uint64 actual_bytes_processed = asm_cache_size_test_256_non_pow_of_2(read_size, read_data, read_block_counts[test_i]);
            shm_repetition_test_end_timer(&tester);
            shm_repetition_test_add_bytes_processed(&tester, actual_bytes_processed);
        }

        shm_repetition_tester_print_last_test_results(&tester);
        test_i = (test_i + 1) % test_count;
    }

    shm_platform_memory_free(buffer.data);
}