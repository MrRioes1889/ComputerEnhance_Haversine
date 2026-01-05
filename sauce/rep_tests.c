#include "rep_tests.h"
#include "shm_utils/shm_repetition_tester.h"
#include "shm_utils/platform/shm_platform.h"
#include <malloc.h>
#include <stdio.h>

#ifdef _WIN32
#pragma comment(lib, "win64_test_loops.lib")
#include <windows.h>
#endif

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

void run_cache_size_tests_pow_2(uint64 time_counter_frequency)
{
    TestBuffer buffer = {0};
    buffer.size = GIBIBYTE(1);
    buffer.data = shm_platform_memory_allocate(buffer.size + 64);
    if (!buffer.data)
        return;

    uint8* read_data = buffer.data;
    // NOTE: For testing unaligned reads
    uint64 read_alignment_offset = 0;
    read_data += read_alignment_offset; //
    // NOTE: Preventing page faults
    for (uint64 i = 0; i < buffer.size; i++)
        read_data[i] = (uint8)i;

    char test_title[256];
    char row_label[64];
    uint64 slice_size = KIBIBYTE(1);
    bool8 running = true;
    SHM_RepetitionTester tester = {0};
    shm_repetition_tester_init(time_counter_frequency, 10.0, false, &tester);
    SHM_RepetitionTesterResultsTable res_table = {0};
    shm_repetition_tester_result_table_alloc(1, 1024, "Slice size", &res_table);
    shm_repetition_tester_result_table_set_column_label(&res_table, 0, "Read speed (gb/s)");
    while (running)
    {
        sprintf_s(test_title, array_count(test_title), "Cache size test 8x32 movs - Slice size: %llu Bytes - Read alignment offset: %llu Bytes", 
            slice_size, read_alignment_offset);
        sprintf_s(row_label, array_count(row_label), "%llukb", slice_size / 1024);
        
        uint32 res_row_index = shm_repetition_tester_result_table_add_row(&res_table, row_label);
        shm_repetition_tester_begin_test(&tester, test_title);

        while (shm_repetition_tester_next_run(&tester))
        {
            shm_repetition_test_begin_timer(&tester);
            asm_cache_size_test_256(buffer.size, read_data, slice_size);
            shm_repetition_test_end_timer(&tester);
            shm_repetition_test_add_bytes_processed(&tester, buffer.size);
        }

        shm_repetition_tester_result_table_add_last_result(&tester, &res_table, 0, res_row_index);
        shm_repetition_tester_print_last_test_results(&tester);
        slice_size *= 2;
        if (slice_size > GIBIBYTE(1))
            running = false;
    }

    shm_repetition_tester_result_table_print_as_csv(&tester, &res_table, SHM_RepetitionTestPrintValueType_GBPerSecond, "cache_size_tests.csv");
    shm_repetition_tester_result_table_free(&res_table);
    shm_platform_memory_free(buffer.data);
}

void run_cache_size_tests_non_pow_2(uint64 time_counter_frequency)
{
    TestBuffer buffer = {0};
    buffer.size = GIBIBYTE(1);
    buffer.data = shm_platform_memory_allocate(buffer.size + 64);
    if (!buffer.data)
        return;

    uint8* read_data = buffer.data;
    // NOTE: For testing unaligned reads
    uint64 read_alignment_offset = 0;
    read_data += read_alignment_offset; //
    // NOTE: Preventing page faults
    for (uint64 i = 0; i < buffer.size; i++)
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
            uint64 actual_bytes_processed = asm_cache_size_test_256_non_pow_of_2(buffer.size, read_data, read_block_counts[test_i]);
            shm_repetition_test_end_timer(&tester);
            shm_repetition_test_add_bytes_processed(&tester, actual_bytes_processed);
        }

        shm_repetition_tester_print_last_test_results(&tester);
        test_i = (test_i + 1) % test_count;
    }

    shm_platform_memory_free(buffer.data);
}

static inline TestBuffer _allocate_filebuffer(const char* filename)
{
    TestBuffer buffer = {0};
    uint64 filesize = shm_platform_get_filesize(filename);
    if (!filesize)
        return buffer;

    buffer.size = filesize;
    buffer.data = shm_platform_memory_allocate(buffer.size);
    return buffer;
}

static inline void _free_filebuffer(TestBuffer* buffer)
{
    free(buffer->data);
    buffer->data = 0;
    buffer->size = 0;
}

static void _test_file_read_fread(SHM_RepetitionTester* tester, const char* filename, uint64 buffer_size, void* buffer)
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
        shm_repetition_test_log_error(tester, "fread x2 failed");
    }
    
    fclose(file);
}

static void _test_file_read_shm_platform(SHM_RepetitionTester* tester, const char* filename, uint64 buffer_size, void* buffer)
{
    SHM_FileHandle file = shm_platform_file_open(filename, false);
    if(!file.handle)
    {
        shm_repetition_test_log_error(tester, "shm_platform fileread failed");
        return;
    }

    shm_repetition_test_begin_timer(tester);
    uint32 result = shm_platform_file_read(file, buffer, (uint32)buffer_size, 0);
    shm_repetition_test_end_timer(tester);
    
    if((uint64)result == buffer_size)
    {
        shm_repetition_test_add_bytes_processed(tester, buffer_size);
    }
    else
    {
        shm_repetition_test_log_error(tester, "shm_platform fileread failed");
    }
    
    shm_platform_file_close(&file);
}

static void _test_file_read_win32_overlapped_x2(SHM_RepetitionTester* tester, const char* filename, uint64 buffer_size, void* buffer)
{
    HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);
    if(!file)
    {
        shm_repetition_test_log_error(tester, "win32 overlapped x2 fileread failed");
        return;
    }

    uint8* buffer1 = buffer;
    uint32 buffer_size1 = (uint32)buffer_size / 2;
    uint8* buffer2 = buffer1 + buffer_size1;
    uint32 buffer_size2 = (uint32)buffer_size - buffer_size1;

    OVERLAPPED overlapped1 = {.Offset = 0, .OffsetHigh = 0};
    OVERLAPPED overlapped2 = {.Offset = buffer_size1, .OffsetHigh = 0};

    shm_repetition_test_begin_timer(tester);
    ReadFile(file, buffer1, buffer_size1, 0, &overlapped1);
    ReadFile(file, buffer2, buffer_size2, 0, &overlapped2);

    uint32 result1, result2;
    GetOverlappedResult(file, &overlapped1, (LPDWORD)&result1, true);
    GetOverlappedResult(file, &overlapped2, (LPDWORD)&result2, true);
    shm_repetition_test_end_timer(tester);
    
    if(((uint64)result1 + (uint64)result2) == buffer_size)
    {
        shm_repetition_test_add_bytes_processed(tester, buffer_size);
    }
    else
    {
        shm_repetition_test_log_error(tester, "shm_platform x2 fileread failed");
    }
    
    CloseHandle(file);
}

typedef void(*FP_raw_file_read_test_function)(SHM_RepetitionTester* tester, const char* filename, uint64 buffer_size, void* buffer);
typedef struct RawFileReadTest
{
    const char* name;
    FP_raw_file_read_test_function func;
}
RawFileReadTest;

void run_file_read_tests(uint64 time_counter_frequency, const char* filename)
{
    TestBuffer file_buffer = _allocate_filebuffer(filename);
    if (!file_buffer.data)
        return;
    
    uint8* read_data = file_buffer.data;
    // NOTE: Preventing page faults
    for (uint64 i = 0; i < file_buffer.size; i++)
        read_data[i] = (uint8)i;

    RawFileReadTest tests[] =
    {
        {.func = _test_file_read_fread, .name = "fread"},
        {.func = _test_file_read_shm_platform, .name = "shm_platform_file_read"},
        {.func = _test_file_read_win32_overlapped_x2, .name = "win32_overlapped_x2"}
    };
    uint32 test_count = array_count(tests);

    char test_title[256];
    SHM_RepetitionTester tester = {0};
    shm_repetition_tester_init(time_counter_frequency, 10.0, false, &tester);
    for (uint32 test_i = 0; test_i < test_count; test_i++)
    {
        sprintf_s(test_title, array_count(test_title), "Read file test - File size: %llu Bytes - Method: %s", 
            file_buffer.size, tests[test_i].name);
        shm_repetition_tester_begin_test(&tester, test_title);

        while (shm_repetition_tester_next_run(&tester))
        {
            tests[test_i].func(&tester, filename, file_buffer.size, file_buffer.data);
        }

        shm_repetition_tester_print_last_test_results(&tester);
    }
}

#define OS_MEM_PAGE_SIZE 4096
static bool8 _test_allocate_and_touch_buffer(SHM_RepetitionTester* tester, const char* filename, uint64 file_size, uint64 read_buffer_size, uint8* scratch_buffer)
{
    uint8* read_buffer = shm_platform_memory_allocate(read_buffer_size);
    if (!read_buffer)
        return false;

    uint64 touch_count = (read_buffer_size + OS_MEM_PAGE_SIZE - 1) / OS_MEM_PAGE_SIZE;
    for (uint64 touch_i = 0; touch_i < touch_count; touch_i++)
        read_buffer[OS_MEM_PAGE_SIZE * touch_i] = (uint8)touch_i;

    shm_repetition_test_add_bytes_processed(tester, file_size);

    shm_platform_memory_free(read_buffer);
    return true;
}

static bool8 _test_allocate_and_copy_buffer(SHM_RepetitionTester* tester, const char* filename, uint64 file_size, uint64 read_buffer_size, uint8* scratch_buffer)
{
    uint8* read_buffer = shm_platform_memory_allocate(read_buffer_size);
    if (!read_buffer)
        return false;

    uint8* src = scratch_buffer;
    uint64 size_remaining = file_size;
    while (size_remaining)
    {
        uint64 read_size = read_buffer_size;
        if (read_size > size_remaining)
            read_size = size_remaining;

        memcpy(read_buffer, src, read_size);
        shm_repetition_test_add_bytes_processed(tester, read_size);
        
        size_remaining -= read_size;
        src += read_size;
    }

    shm_platform_memory_free(read_buffer);
    return true;
}

static bool8 _test_allocate_and_win32_read_file(SHM_RepetitionTester* tester, const char* filename, uint64 file_size, uint64 read_buffer_size, uint8* scratch_buffer)
{
    HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE)
        return false;

    uint8* read_buffer = shm_platform_memory_allocate(read_buffer_size);
    if (!read_buffer)
    {
        CloseHandle(file);
        return false;
    }

    uint64 size_remaining = file_size;
    while (size_remaining)
    {
        uint64 read_size = read_buffer_size;
        if (read_size > size_remaining)
            read_size = size_remaining;

        DWORD bytes_read = 0;
        BOOL res = ReadFile(file, read_buffer, (DWORD)read_size, &bytes_read, 0);

        if (res && (bytes_read == read_size))
            shm_repetition_test_add_bytes_processed(tester, read_size);
        else
            shm_repetition_test_log_error(tester, "ReadFile failed.");
        
        size_remaining -= read_size;
    }

    shm_platform_memory_free(read_buffer);
    CloseHandle(file);
    return true;
}

static bool8 _test_allocate_and_fread(SHM_RepetitionTester* tester, const char* filename, uint64 file_size, uint64 read_buffer_size, uint8* scratch_buffer)
{
    FILE* file = fopen(filename, "rb");
    if (!file)
        return false;

    uint8* read_buffer = shm_platform_memory_allocate(read_buffer_size);
    if (!read_buffer)
    {
        fclose(file);
        return false;
    }

    uint64 size_remaining = file_size;
    while (size_remaining)
    {
        uint64 read_size = read_buffer_size;
        if (read_size > size_remaining)
            read_size = size_remaining;

        uint64 res = fread(read_buffer, read_size, 1, file);

        if (res == 1)
            shm_repetition_test_add_bytes_processed(tester, read_size);
        else
            shm_repetition_test_log_error(tester, "fread failed.");
        
        size_remaining -= read_size;
    }

    shm_platform_memory_free(read_buffer);
    fclose(file);
    return true;
}

typedef bool8(*FP_incremental_file_read_test_function)(SHM_RepetitionTester* tester, const char* filename, uint64 file_size, uint64 read_buffer_size, uint8* scratch_buffer);
typedef struct IncrementalFileReadTest
{
    const char* name;
    FP_incremental_file_read_test_function func;
}
IncrementalFileReadTest;

void run_incremental_file_read_tests(uint64 time_counter_frequency, const char* filename)
{
    TestBuffer scratch_buffer = _allocate_filebuffer(filename);
    if (!scratch_buffer.data)
        return;
    
    uint8* read_data = scratch_buffer.data;
    // NOTE: Preventing page faults
    for (uint64 i = 0; i < scratch_buffer.size; i++)
        read_data[i] = (uint8)i;

    IncrementalFileReadTest tests[] =
    {
        {.func = _test_allocate_and_touch_buffer, .name = "Allocate & Touch"},
        {.func = _test_allocate_and_copy_buffer, .name = "Allocate & Copy"},
        {.func = _test_allocate_and_win32_read_file, .name = "Allocate & ReadFile"},
        {.func = _test_allocate_and_fread, .name = "Allocate & fread"}
    };
    uint32 test_count = array_count(tests);

    char test_title[256];
    char row_label[64];
    SHM_RepetitionTester tester = {0};
    shm_repetition_tester_init(time_counter_frequency, 10.0, false, &tester);
    SHM_RepetitionTesterResultsTable res_table = {0};
    shm_repetition_tester_result_table_alloc(test_count, 1024, "Read Buffer Size", &res_table);
    for(uint32 test_i = 0; test_i < test_count; test_i++)
        shm_repetition_tester_result_table_set_column_label(&res_table, test_i, tests[test_i].name);

    uint32 row_i = 0;
    for (uint64 read_buffer_size = KIBIBYTE(256); read_buffer_size <= scratch_buffer.size; read_buffer_size *= 2)
    {
        sprintf_s(row_label, array_count(row_label), "%llukb", read_buffer_size / 1024) ;
        shm_repetition_tester_result_table_add_row(&res_table, row_label);

        for(uint32 test_i = 0; test_i < test_count; test_i++)
        {
            sprintf_s(test_title, array_count(test_title), "%s - Read buffer size: %llu Bytes", 
                tests[test_i].name, read_buffer_size);
            shm_repetition_tester_begin_test(&tester, test_title);

            while (shm_repetition_tester_next_run(&tester))
            {
                shm_repetition_test_begin_timer(&tester);
                tests[test_i].func(&tester, filename, scratch_buffer.size, read_buffer_size, scratch_buffer.data);
                shm_repetition_test_end_timer(&tester);
            }

            shm_repetition_tester_result_table_add_last_result(&tester, &res_table, test_i, row_i);
            shm_repetition_tester_print_last_test_results(&tester);
        }

        row_i++;
    }

    shm_repetition_tester_result_table_print_as_csv(&tester, &res_table, SHM_RepetitionTestPrintValueType_GBPerSecond, "incremental_file_read_tests.csv");
    shm_repetition_tester_result_table_free(&res_table);
}

static void _test_write_all_bytes(uint64 buffer_size, uint8* buffer)
{
    for (uint64 i = 0; i < buffer_size; i++)
    {
        buffer[i] = (uint8)i;
    }
}