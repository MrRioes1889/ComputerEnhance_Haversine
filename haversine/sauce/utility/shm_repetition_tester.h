#pragma once
#include "defines.h"

enum SHM_RepetitionTestResultType
{
    SHM_RepetitionTestResultType_TSC,
    SHM_RepetitionTestResultType_BytesProcessed,
    SHM_RepetitionTestResultType_PageFaultCount,

    SHM_RepetitionTestResultTypeCount
};

typedef struct
{
    uint64 max[SHM_RepetitionTestResultTypeCount];
    uint64 min[SHM_RepetitionTestResultTypeCount];
    uint64 total[SHM_RepetitionTestResultTypeCount];
}
SHM_RepetitionTestResults;

struct SHM_RepetitionTester;
typedef void (*FP_SHM_RepetitionTestFunc)(struct SHM_RepetitionTester* tester, void* params);
typedef struct
{
    const char* name;
    FP_SHM_RepetitionTestFunc func;
}
SHM_RepetitionTest;

typedef struct SHM_RepetitionTester
{
    uint32 test_count;
    uint32 current_test_id;
    SHM_RepetitionTest* tests;
    float64 min_test_runtime_seconds;
    uint64 time_counter_frequency;
    uint64 run_count;
    uint64 run_results[SHM_RepetitionTestResultTypeCount];
    SHM_RepetitionTestResults test_results;
}
SHM_RepetitionTester;

bool8 shm_repetition_tester_init(SHM_RepetitionTest* tests, uint32 test_count, uint64 time_counter_frequency, float64 min_test_runtime_seconds, SHM_RepetitionTester* out_tester);

bool8 shm_repetition_tester_run_next_test(SHM_RepetitionTester* tester, void* params);
void shm_repetition_test_begin_timer(SHM_RepetitionTester* tester);
void shm_repetition_test_end_timer(SHM_RepetitionTester* tester);
void shm_repetition_test_add_bytes_processed(SHM_RepetitionTester* tester, uint64 bytes_count);
void shm_repetition_test_log_error(SHM_RepetitionTester* tester, const char* msg);

