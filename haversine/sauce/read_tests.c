#include "read_tests.h"
#include "utility/shm_repetition_tester.h"
#include "platform/shm_platform.h"
#include <malloc.h>
#include <stdio.h>

typedef struct 
{
    uint64 size;
    void* data;
}
ReadTestBuffer;

typedef struct
{
    const char* filename;
    ReadTestBuffer preallocated_buffer;
}
ReadTestParams;

static inline ReadTestBuffer _allocate_filebuffer(const char* filename)
{
    ReadTestBuffer buffer = {0};
    uint64 filesize = shm_platform_get_filesize(filename);
    if (!filesize)
        return buffer;

    buffer.size = filesize;
    buffer.data = malloc(buffer.size);
    return buffer;
}

static inline void _free_filebuffer(ReadTestBuffer* buffer)
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

static void _test_fread_preallocated(SHM_RepetitionTester* tester, void* _params)
{
    ReadTestParams* params = (ReadTestParams*)_params;
    _test_fread(tester, params->filename, params->preallocated_buffer.data, params->preallocated_buffer.size);
}

static void _test_fread_alloc(SHM_RepetitionTester* tester, void* _params)
{
    ReadTestParams* params = (ReadTestParams*)_params;
    ReadTestBuffer buffer = _allocate_filebuffer(params->filename);
    if (!buffer.data)
    {
        shm_repetition_test_log_error(tester, "Failed to get filesize for allocation.");
        return;
    }

    _test_fread(tester, params->filename, buffer.data, buffer.size);
    _free_filebuffer(&buffer);
}

void run_all_read_tests(const char* filename, uint64 time_counter_frequency)
{
    ReadTestParams params = {0};
    params.filename = filename;
    params.preallocated_buffer = _allocate_filebuffer(params.filename);
    if (!params.preallocated_buffer.data)
        return;

    #define test_count 2
    SHM_RepetitionTest tests[test_count] =
    {
        {.func = _test_fread_preallocated, .name = "fread preallocated"},
        {.func = _test_fread_alloc, .name = "fread + alloc"}
    };

    bool8 running = true;
    SHM_RepetitionTester tester = {0};
    shm_repetition_tester_init(tests, test_count, time_counter_frequency, 10.0, &tester);
    while (running && shm_repetition_tester_run_next_test(&tester, &params))
    {

    }
}


