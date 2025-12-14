#include "shm_repetition_tester.h"

#include "platform/shm_platform.h"
#include "platform/shm_intrin.h"
#include <stdio.h>

#define SHM_REPETITION_TESTER_GET_PAGE_FAULT_COUNT() shm_platform_metrics_get_page_fault_count()
#define SHM_REPETITION_TESTER_GET_TSC() shm_intrin_rdtsc()

#ifndef SHM_REPETITION_TESTER_GET_TSC
#ifdef SHM_UTILS_GENERAL_GET_TSC
#define SHM_REPETITION_TESTER_GET_TSC() SHM_UTILS_GENERAL_GET_TSC()
#else
#define SHM_REPETITION_TESTER_GET_TSC() 0
static_assert(false, "SHM Repetition tester needs to have SHM_REPETITION_TESTER_GET_TSC() macro defined that returns uint64.")
#endif
#endif

#ifndef SHM_REPETITION_TESTER_GET_PAGE_FAULT_COUNT
#ifdef SHM_UTILS_GENERAL_GET_PAGE_FAULT_COUNT
#define SHM_REPETITION_TESTER_GET_PAGE_FAULT_COUNT() SHM_UTILS_GENERAL_GET_PAGE_FAULT_COUNT()
#else
#define SHM_REPETITION_TESTER_GET_PAGE_FAULT_COUNT() 0
#endif
#endif

#define SHM_UTILS_GENERAL_ENABLE_LOGGING 1
#if SHM_UTILS_GENERAL_ENABLE_LOGGING

#ifndef SHM_REPETITION_TESTER_LOG
#ifdef SHM_UTILS_GENERAL_LOG
#define SHM_REPETITION_TESTER_LOG(msg, ...) SHM_UTILS_GENERAL_LOG_V(msg, __VA_ARGS__)
#else
#include <stdio.h>
#define SHM_REPETITION_TESTER_LOG(msg, ...) fprintf_s(stdout, msg, ##__VA_ARGS__)
#endif
#endif

#ifndef SHM_REPETITION_TESTER_LOG_ERR
#ifdef SHM_UTILS_GENERAL_LOG_ERR
#define SHM_REPETITION_TESTER_LOG_ERR(msg, ...) SHM_UTILS_GENERAL_LOG_ERR(msg, __VA_ARGS__)
#else
#include <stdio.h>
#define SHM_REPETITION_TESTER_LOG_ERR(msg, ...) fprintf_s(stderr, msg, ##__VA_ARGS__)
#endif
#endif

#else
#define SHM_REPETITION_TESTER_LOG(msg, ...)
#define SHM_REPETITION_TESTER_LOG_ERR(msg, ...)
#endif

static inline float64 _tsc_to_seconds(uint64 tsc_frequency, uint64 tsc_elapsed)
{
    return ((float64)tsc_elapsed / (float64)tsc_frequency);
}

static void _print_result_line(const char* label, uint64* values, uint64 invalid_value, uint64 tsc_frequency)
{
    float64 kb = 1024.0;
    float64 mb = kb * 1024.0;
    float64 gb = mb * 1024.0;

    uint64 tsc = values[SHM_RepetitionTestResultType_TSC];
    uint64 bytes = values[SHM_RepetitionTestResultType_BytesProcessed];
    uint64 p_faults = values[SHM_RepetitionTestResultType_PageFaultCount];
    float64 seconds = _tsc_to_seconds(tsc_frequency, tsc);

    SHM_REPETITION_TESTER_LOG("%s: Time: %.5fms (%llu), Data: %lluB (%.5fgb/s), Page Faults: %llu (%.5fkb/fault)\n", 
        label, 
        seconds * 1000.0, tsc, 
        bytes, tsc == invalid_value ? 0.0 : (((float64)bytes / gb) / seconds), 
        p_faults, p_faults == 0 ? 0.0 : ((float64)bytes / ((float64)p_faults * kb)));
}

static void _print_test_results(SHM_RepetitionTester* tester, float64 total_seconds_elapsed)
{
    SHM_REPETITION_TESTER_LOG("Run count: %u, Total time elapsed: %.5fs\n", tester->run_count, total_seconds_elapsed);

    uint64 averages[SHM_RepetitionTestResultTypeCount];
    for (uint32 type_i = 0; type_i < SHM_RepetitionTestResultTypeCount; type_i++)
        averages[type_i] = tester->test_results.total[type_i] / tester->run_count;

    _print_result_line("Min", tester->test_results.min, UINT64_MAX, tester->time_counter_frequency);
    _print_result_line("Max", tester->test_results.max, 0, tester->time_counter_frequency);
    _print_result_line("Avg", averages, 0, tester->time_counter_frequency);
    SHM_REPETITION_TESTER_LOG("\n");
}

bool8 shm_repetition_tester_init(SHM_RepetitionTest* tests, uint32 test_count, uint64 time_counter_frequency, float64 min_test_runtime_seconds, SHM_RepetitionTester* out_tester)
{
    out_tester->tests = tests;
    out_tester->test_count = test_count;
    if (time_counter_frequency == 0)
    {
        SHM_REPETITION_TESTER_LOG_ERR("Error: Repetition Tester: time counter frequency has to be larger than 0.");
        return false;
    }
    out_tester->time_counter_frequency = time_counter_frequency;
    out_tester->min_test_runtime_seconds = min_test_runtime_seconds;
    out_tester->current_test_id = test_count - 1;
    return true;
}

bool8 shm_repetition_tester_run_next_test(SHM_RepetitionTester* tester, void* params)
{
    tester->current_test_id = (tester->current_test_id + 1) % tester->test_count;
    SHM_RepetitionTest* test = tester->tests + tester->current_test_id;

    tester->run_count = 0;
    for (uint32 type_i = 0; type_i < SHM_RepetitionTestResultTypeCount; type_i++)
    {
        tester->run_results[type_i] = 0;
        tester->test_results.min[type_i] = UINT64_MAX;
        tester->test_results.max[type_i] = 0;
        tester->test_results.total[type_i] = 0;
    }

    SHM_REPETITION_TESTER_LOG("Running repetition test '%s':\n", test->name);
    uint64 total_tsc_start = SHM_REPETITION_TESTER_GET_TSC();
    float64 total_seconds_elapsed = 0.0;
    while (total_seconds_elapsed < tester->min_test_runtime_seconds)
    {
        test->func((struct SHM_RepetitionTester*)tester, params);

        for (uint32 type_i = 0; type_i < SHM_RepetitionTestResultTypeCount; type_i++)
        {
            uint64 run_result = tester->run_results[type_i];
            tester->run_results[type_i] = 0;
            SHM_RepetitionTestResults* test_result = &tester->test_results;
            test_result->total[type_i] += run_result;
            if (run_result < test_result->min[type_i])
                test_result->min[type_i] = run_result;
            if (run_result > test_result->max[type_i])
                test_result->max[type_i] = run_result;
        }

        tester->run_count++;
        SHM_REPETITION_TESTER_LOG("Run count: %u, Total time elapsed: %.5fs, Current minimal time: %.5fms\r",
            tester->run_count, total_seconds_elapsed, _tsc_to_seconds(tester->time_counter_frequency, tester->test_results.min[SHM_RepetitionTestResultType_TSC]) * 1000.0);
        total_seconds_elapsed = _tsc_to_seconds(tester->time_counter_frequency, (SHM_REPETITION_TESTER_GET_TSC() - total_tsc_start));
    }

    _print_test_results(tester, total_seconds_elapsed);
    return true;
}

void shm_repetition_test_begin_timer(SHM_RepetitionTester* tester)
{
    tester->run_results[SHM_RepetitionTestResultType_TSC] -= SHM_REPETITION_TESTER_GET_TSC();
    tester->run_results[SHM_RepetitionTestResultType_PageFaultCount] -= SHM_REPETITION_TESTER_GET_PAGE_FAULT_COUNT();
}

void shm_repetition_test_end_timer(SHM_RepetitionTester* tester)
{
    tester->run_results[SHM_RepetitionTestResultType_TSC] += SHM_REPETITION_TESTER_GET_TSC();
    tester->run_results[SHM_RepetitionTestResultType_PageFaultCount] += SHM_REPETITION_TESTER_GET_PAGE_FAULT_COUNT();
}

void shm_repetition_test_add_bytes_processed(SHM_RepetitionTester* tester, uint64 bytes_count)
{
    tester->run_results[SHM_RepetitionTestResultType_BytesProcessed] += bytes_count;
}

void shm_repetition_test_log_error(SHM_RepetitionTester* tester, const char* msg)
{
    SHM_REPETITION_TESTER_LOG_ERR("Error: Repetition Tester: %s\n", msg);
}