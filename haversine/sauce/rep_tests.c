#include "rep_tests.h"
#include "utility/shm_repetition_tester.h"
#include "platform/shm_platform.h"
#include <malloc.h>
#include <stdio.h>

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

static void _test_fread(SHM_RepetitionTester* tester, const char* filename, void* buffer, uint64 buffer_size)
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

static void _test_fread_prealloc(SHM_RepetitionTester* tester, void* _params)
{
    TestParams* params = (TestParams*)_params;
    _test_fread(tester, params->filename, params->preallocated_buffer.data, params->preallocated_buffer.size);
}

static void _test_fread_alloc(SHM_RepetitionTester* tester, void* _params)
{
    TestParams* params = (TestParams*)_params;
    TestBuffer buffer = _allocate_filebuffer(params->filename);
    if (!buffer.data)
    {
        shm_repetition_test_log_error(tester, "Failed to get filesize for allocation.");
        return;
    }

    _test_fread(tester, params->filename, buffer.data, buffer.size);
    _free_filebuffer(&buffer);
}

static void _test_write_all_bytes(SHM_RepetitionTester* tester, uint8* buffer, uint64 buffer_size)
{
    shm_repetition_test_begin_timer(tester);
    for (uint64 i = 0; i < buffer_size; i++)
    {
        buffer[i] = (uint8)i;
    }
    shm_repetition_test_end_timer(tester);

    shm_repetition_test_add_bytes_processed(tester, buffer_size);
}

static void _test_write_all_bytes_prealloc(SHM_RepetitionTester* tester, void* _params)
{
    TestParams* params = (TestParams*)_params;
    _test_write_all_bytes(tester, params->preallocated_buffer.data, params->preallocated_buffer.size);
}

static void _test_write_all_bytes_alloc(SHM_RepetitionTester* tester, void* _params)
{
    TestParams* params = (TestParams*)_params;
    TestBuffer buffer = _allocate_filebuffer(params->filename);
    if (!buffer.data)
    {
        shm_repetition_test_log_error(tester, "Failed to get filesize for allocation.");
        return;
    }

    _test_write_all_bytes(tester, buffer.data, buffer.size);
    _free_filebuffer(&buffer);
}

void run_all_tests(const char* filename, uint64 time_counter_frequency)
{
    TestParams params = {0};
    params.filename = filename;
    params.preallocated_buffer = _allocate_filebuffer(params.filename);
    if (!params.preallocated_buffer.data)
        return;

    #define test_count 1
    SHM_RepetitionTest tests[test_count] =
    {
        {.func = _test_write_all_bytes_prealloc, .name = "write all bytes preallocated"},
    };

    bool8 running = true;
    SHM_RepetitionTester tester = {0};
    shm_repetition_tester_init(tests, test_count, time_counter_frequency, 10.0, &tester);
    while (running && shm_repetition_tester_run_next_test(&tester, &params))
    {

    }
}


