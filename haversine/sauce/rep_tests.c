#include "rep_tests.h"
#include "utility/shm_repetition_tester.h"
#include "platform/shm_platform.h"
#include <malloc.h>
#include <stdio.h>

#pragma comment(lib, "test_write_all_bytes.lib")
extern void asm_write_all_bytes_mov(uint64 buffer_size, uint8* buffer);
extern void asm_write_all_bytes_nop(uint64 buffer_size, uint8* buffer);
extern void asm_write_all_bytes_cmp(uint64 buffer_size, uint8* buffer);
extern void asm_write_all_bytes_dec(uint64 buffer_size, uint8* buffer);

#define SINGLE_FUNCTION_TEST(test_fun_name, params_type, bytes_variable, fun_call) \
static void test_fun_name(SHM_RepetitionTester* tester, void* _params)\
{\
    params_type* params = (params_type*)_params;\
    shm_repetition_test_begin_timer(tester);\
    fun_call;\
    shm_repetition_test_end_timer(tester);\
    shm_repetition_test_add_bytes_processed(tester, bytes_variable);\
}

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

static void __test_fread(SHM_RepetitionTester* tester, const char* filename, void* buffer, uint64 buffer_size)
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
    __test_fread(tester, params->filename, params->preallocated_buffer.data, params->preallocated_buffer.size);
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

    __test_fread(tester, params->filename, buffer.data, buffer.size);
    _free_filebuffer(&buffer);
}

static void _write_all_bytes(uint64 buffer_size, uint8* buffer)
{
    for (uint64 i = 0; i < buffer_size; i++)
    {
        buffer[i] = (uint8)i;
    }
}

static void _test_write_all_bytes_c(SHM_RepetitionTester* tester, void* _params)
{
    TestParams* params = (TestParams*)_params;
    shm_repetition_test_begin_timer(tester);
    _write_all_bytes(params->preallocated_buffer.size, params->preallocated_buffer.data);
    shm_repetition_test_end_timer(tester);

    shm_repetition_test_add_bytes_processed(tester, params->preallocated_buffer.size);
}

static void _test_write_all_bytes_asm_mov(SHM_RepetitionTester* tester, void* _params)
{
    TestParams* params = (TestParams*)_params;
    shm_repetition_test_begin_timer(tester);
    asm_write_all_bytes_mov(params->preallocated_buffer.size, params->preallocated_buffer.data);
    shm_repetition_test_end_timer(tester);

    shm_repetition_test_add_bytes_processed(tester, params->preallocated_buffer.size);
}

static void _test_loop_all_bytes_asm_nop(SHM_RepetitionTester* tester, void* _params)
{
    TestParams* params = (TestParams*)_params;
    shm_repetition_test_begin_timer(tester);
    asm_write_all_bytes_nop(params->preallocated_buffer.size, params->preallocated_buffer.data);
    shm_repetition_test_end_timer(tester);

    shm_repetition_test_add_bytes_processed(tester, params->preallocated_buffer.size);
}

static void _test_loop_all_bytes_asm_cmp(SHM_RepetitionTester* tester, void* _params)
{
    TestParams* params = (TestParams*)_params;
    shm_repetition_test_begin_timer(tester);
    asm_write_all_bytes_cmp(params->preallocated_buffer.size, params->preallocated_buffer.data);
    shm_repetition_test_end_timer(tester);

    shm_repetition_test_add_bytes_processed(tester, params->preallocated_buffer.size);
}

static void _test_loop_all_bytes_asm_dec(SHM_RepetitionTester* tester, void* _params)
{
    TestParams* params = (TestParams*)_params;
    shm_repetition_test_begin_timer(tester);
    asm_write_all_bytes_dec(params->preallocated_buffer.size, params->preallocated_buffer.data);
    shm_repetition_test_end_timer(tester);

    shm_repetition_test_add_bytes_processed(tester, params->preallocated_buffer.size);
}

typedef void(*FP_test_function)(uint64 buffer_size, uint8* buffer);
typedef struct TestFunction
{
    const char* name;
    FP_test_function func;
}
TestFunction;

void run_all_tests(const char* filename, uint64 time_counter_frequency)
{
    TestBuffer buffer = _allocate_filebuffer(filename);
    if (!buffer.data)
        return;

    TestFunction tests[5] =
    {
        {.func = _write_all_bytes, .name = "Write All Bytes - C"},
        {.func = asm_write_all_bytes_mov, .name = "Write All Bytes - ASM"},
        {.func = asm_write_all_bytes_nop, .name = "Nop All Bytes - ASM"},
        {.func = asm_write_all_bytes_cmp, .name = "Cmp All Bytes - ASM",},
        {.func = asm_write_all_bytes_dec, .name = "Dec All Bytes - ASM",}
    };
    uint32 test_count = array_count(tests);

    bool8 running = true;
    SHM_RepetitionTester tester = {0};
    shm_repetition_tester_init(time_counter_frequency, 10.0, &tester);
    uint32 test_i = test_count - 1;
    while (running)
    {
        test_i = (test_i + 1) % test_count;
        shm_repetition_tester_begin_test(&tester, tests[test_i].name);

        while (shm_repetition_tester_next_run(&tester))
        {
            shm_repetition_test_begin_timer(&tester);
            tests[test_i].func(buffer.size, buffer.data);
            shm_repetition_test_end_timer(&tester);
            shm_repetition_test_add_bytes_processed(&tester, buffer.size);
        }

        shm_repetition_tester_print_last_test_results(&tester);
    }
}


