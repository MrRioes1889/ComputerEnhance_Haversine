#pragma once
#include "shm_utils_common_defines.h"

enum SHM_RepetitionTestResultType
{
    SHM_RepetitionTestResultType_TSC,
    SHM_RepetitionTestResultType_BytesProcessed,
    SHM_RepetitionTestResultType_PageFaultCount,

    SHM_RepetitionTestResultType_ENUMCOUNT
};

typedef struct
{
    uint64 run_count;
    uint64 max[SHM_RepetitionTestResultType_ENUMCOUNT];
    uint64 min[SHM_RepetitionTestResultType_ENUMCOUNT];
    uint64 total[SHM_RepetitionTestResultType_ENUMCOUNT];
}
SHM_RepetitionTestResults;

typedef struct SHM_RepetitionTesterResultsTableLabel
{
    char text[64];
}
SHM_RepetitionTesterResultsTableLabel;

typedef struct SHM_RepetitionTesterResultsTable
{
    uint32 max_row_count;
    uint32 column_count;
    uint32 row_count;

    SHM_RepetitionTesterResultsTableLabel row_label_label;
    SHM_RepetitionTesterResultsTableLabel* column_labels;
    SHM_RepetitionTesterResultsTableLabel* row_labels;
    SHM_RepetitionTestResults* results;
}
SHM_RepetitionTesterResultsTable;

typedef enum SHM_RepetitionTestPrintValueType
{
    SHM_RepetitionTestPrintValueType_GBPerSecond,

    SHM_RepetitionTestPrintValueType_ENUMCOUNT
}
SHM_RepetitionTestPrintValueType;

typedef struct SHM_RepetitionTester
{
    float64 test_runtime_duration;
    uint64 time_counter_frequency;
    uint64 tsc_last_test_start;
    uint64 run_results[SHM_RepetitionTestResultType_ENUMCOUNT];
    SHM_RepetitionTestResults test_results;
    bool8 log_stats_during_test;
}
SHM_RepetitionTester;

bool8 shm_repetition_tester_init(uint64 time_counter_frequency, float64 test_runtime_duration, bool8 log_stats_during_test, SHM_RepetitionTester* out_tester);

bool8 shm_repetition_tester_begin_test(SHM_RepetitionTester* tester, const char* test_name);
bool8 shm_repetition_tester_next_run(SHM_RepetitionTester* tester);
void shm_repetition_tester_print_last_test_results(SHM_RepetitionTester* tester);
void shm_repetition_test_begin_timer(SHM_RepetitionTester* tester);
void shm_repetition_test_end_timer(SHM_RepetitionTester* tester);
void shm_repetition_test_add_bytes_processed(SHM_RepetitionTester* tester, uint64 bytes_count);
void shm_repetition_test_log_error(SHM_RepetitionTester* tester, const char* msg);

bool8 shm_repetition_tester_result_table_alloc(uint32 column_count, uint32 max_row_count, const char* row_label_label, SHM_RepetitionTesterResultsTable* out_table);
void shm_repetition_tester_result_table_free(SHM_RepetitionTesterResultsTable* table);
void shm_repetition_tester_result_table_set_column_label(SHM_RepetitionTesterResultsTable* table, uint32 col_index, const char* label);
uint32 shm_repetition_tester_result_table_add_row(SHM_RepetitionTesterResultsTable* table, const char* row_label);
void shm_repetition_tester_result_table_add_last_result(SHM_RepetitionTester* tester, SHM_RepetitionTesterResultsTable* table, uint32 col_index, uint32 row_index);
void shm_repetition_tester_result_table_print_as_csv(SHM_RepetitionTester* tester, SHM_RepetitionTesterResultsTable* table, SHM_RepetitionTestPrintValueType print_value_type, const char* out_csv_filepath);

#ifdef SHM_REPETITION_TESTER_IMPL

#include <string.h>
#include <stdio.h>

#if !defined(SHM_REPETITION_TESTER_ALLOC) || !defined(SHM_REPETITION_TESTER_FREE)
#if defined(SHM_UTILS_GENERAL_ALLOC) && defined(SHM_UTILS_GENERAL_FREE)
#define SHM_REPETITION_TESTER_ALLOC(size) SHM_UTILS_GENERAL_ALLOC(size)
#define SHM_REPETITION_TESTER_FREE(block) SHM_UTILS_GENERAL_FREE(block)
#else
#include <malloc.h>
#define SHM_REPETITION_TESTER_ALLOC(size) malloc(size)
#define SHM_REPETITION_TESTER_FREE(block) free(block)
#endif
#endif

#ifndef SHM_REPETITION_TESTER_GET_TSC
#ifdef SHM_UTILS_GENERAL_GET_TSC
#define SHM_REPETITION_TESTER_GET_TSC() SHM_UTILS_GENERAL_GET_TSC()
#else
#define SHM_REPETITION_TESTER_GET_TSC() 0
static_assert(false, "SHM Repetition tester needs to have SHM_REPETITION_TESTER_GET_TSC() macro defined that returns uint64.");
#endif
#endif

#ifndef SHM_REPETITION_TESTER_GET_PAGE_FAULT_COUNT
#ifdef SHM_UTILS_GENERAL_GET_PAGE_FAULT_COUNT
#define SHM_REPETITION_TESTER_GET_PAGE_FAULT_COUNT() SHM_UTILS_GENERAL_GET_PAGE_FAULT_COUNT()
#else
#define SHM_REPETITION_TESTER_GET_PAGE_FAULT_COUNT() 0
#endif
#endif

#if SHM_UTILS_GENERAL_ENABLE_LOGGING

#ifndef SHM_REPETITION_TESTER_LOG
#ifdef SHM_UTILS_GENERAL_LOG
#define SHM_REPETITION_TESTER_LOG(msg, ...) SHM_UTILS_GENERAL_LOG(msg, ##__VA_ARGS__)
#else
#define SHM_REPETITION_TESTER_LOG(msg, ...) fprintf_s(stdout, msg, ##__VA_ARGS__)
#endif
#endif

#ifndef SHM_REPETITION_TESTER_LOG_ERR
#ifdef SHM_UTILS_GENERAL_LOG_ERR
#define SHM_REPETITION_TESTER_LOG_ERR(msg, ...) SHM_UTILS_GENERAL_LOG_ERR(msg, ##__VA_ARGS__)
#else
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

static void _print_test_results(SHM_RepetitionTester* tester)
{
    SHM_REPETITION_TESTER_LOG("Run count: %llu, Total time elapsed: %.5fs                                  \n", tester->test_results.run_count, _tsc_to_seconds(tester->time_counter_frequency, (SHM_REPETITION_TESTER_GET_TSC() - tester->tsc_last_test_start)));

    uint64 averages[SHM_RepetitionTestResultType_ENUMCOUNT];
    for (uint32 type_i = 0; type_i < SHM_RepetitionTestResultType_ENUMCOUNT; type_i++)
        averages[type_i] = tester->test_results.total[type_i] / tester->test_results.run_count;

    _print_result_line("Min", tester->test_results.min, UINT64_MAX, tester->time_counter_frequency);
    _print_result_line("Max", tester->test_results.max, 0, tester->time_counter_frequency);
    _print_result_line("Avg", averages, 0, tester->time_counter_frequency);
    SHM_REPETITION_TESTER_LOG("\n");
}

bool8 shm_repetition_tester_init(uint64 time_counter_frequency, float64 test_runtime_duration, bool8 log_stats_during_test, SHM_RepetitionTester* out_tester)
{
    if (time_counter_frequency == 0)
    {
        SHM_REPETITION_TESTER_LOG_ERR("Error: Repetition Tester: time counter frequency has to be larger than 0.");
        return false;
    }
    out_tester->time_counter_frequency = time_counter_frequency;
    out_tester->test_runtime_duration = test_runtime_duration;
    out_tester->log_stats_during_test = log_stats_during_test;
    return true;
}

bool8 shm_repetition_tester_begin_test(SHM_RepetitionTester* tester, const char* test_name)
{
    tester->test_results.run_count = 0;
    for (uint32 type_i = 0; type_i < SHM_RepetitionTestResultType_ENUMCOUNT; type_i++)
    {
        tester->run_results[type_i] = 0;
        tester->test_results.min[type_i] = UINT64_MAX;
        tester->test_results.max[type_i] = 0;
        tester->test_results.total[type_i] = 0;
    }

    SHM_REPETITION_TESTER_LOG("Running repetition test '%s':\n", test_name);
    tester->tsc_last_test_start = 0;
    return true;
}

bool8 shm_repetition_tester_next_run(SHM_RepetitionTester* tester)
{
    if (tester->tsc_last_test_start == 0)
    {
        tester->tsc_last_test_start = SHM_REPETITION_TESTER_GET_TSC();
        return true;
    }

    for (uint32 type_i = 0; type_i < SHM_RepetitionTestResultType_ENUMCOUNT; type_i++)
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

    tester->test_results.run_count++;
    float64 total_seconds_elapsed = _tsc_to_seconds(tester->time_counter_frequency, (SHM_REPETITION_TESTER_GET_TSC() - tester->tsc_last_test_start));
    if(tester->log_stats_during_test)
    {
        SHM_REPETITION_TESTER_LOG("Run count: %llu, Total time elapsed: %.5fs, Current minimal time: %.5fms\r",
            tester->test_results.run_count, total_seconds_elapsed, _tsc_to_seconds(tester->time_counter_frequency, tester->test_results.min[SHM_RepetitionTestResultType_TSC]) * 1000.0);
    }
    
    return total_seconds_elapsed < tester->test_runtime_duration;
}

void shm_repetition_tester_print_last_test_results(SHM_RepetitionTester* tester)
{
    _print_test_results(tester);
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

bool8 shm_repetition_tester_result_table_alloc(uint32 column_count, uint32 max_row_count, const char* row_label_label, SHM_RepetitionTesterResultsTable* out_table)
{
    out_table->column_count = column_count;
    out_table->max_row_count = max_row_count;
    out_table->row_count = 0;

    uint64 allocation_size = ((column_count + max_row_count) * sizeof(out_table->column_labels[0])) + (column_count * max_row_count * sizeof(out_table->results[0]));
    void* data = SHM_REPETITION_TESTER_ALLOC(allocation_size);
    if (!data)
        return false;

    out_table->column_labels = data;
    out_table->row_labels = out_table->column_labels + column_count;
    out_table->results = (void*)(out_table->row_labels + max_row_count);
    strcpy_s(out_table->row_label_label.text, array_count(out_table->row_label_label.text), row_label_label);
    return true;
}

void shm_repetition_tester_result_table_free(SHM_RepetitionTesterResultsTable* table)
{
    SHM_REPETITION_TESTER_FREE(table->column_labels);
    *table = (SHM_RepetitionTesterResultsTable){0};
}

void shm_repetition_tester_result_table_set_column_label(SHM_RepetitionTesterResultsTable* table, uint32 col_index, const char* label)
{
    if (col_index >= table->column_count)
        return;

    strcpy_s(table->column_labels[col_index].text, array_count(table->column_labels[col_index].text), label);
}

uint32 shm_repetition_tester_result_table_add_row(SHM_RepetitionTesterResultsTable* table, const char* row_label)
{
    if (table->row_count >= table->max_row_count)
        return table->max_row_count - 1;

    uint32 row_index = table->row_count++;
    strcpy_s(table->row_labels[row_index].text, array_count(table->row_labels[row_index].text), row_label);
    return row_index;
}

void shm_repetition_tester_result_table_add_last_result(SHM_RepetitionTester* tester, SHM_RepetitionTesterResultsTable* table, uint32 col_index, uint32 row_index)
{
    if (col_index >= table->column_count || row_index >= table->row_count)
        return;

    table->results[(row_index * table->column_count) + col_index] = tester->test_results;
}

void shm_repetition_tester_result_table_print_as_csv(SHM_RepetitionTester* tester, SHM_RepetitionTesterResultsTable* table, SHM_RepetitionTestPrintValueType print_value_type, const char* out_csv_filepath)
{
    FILE* csv_file = out_csv_filepath ? fopen(out_csv_filepath, "w") : stdout;
    if (!csv_file)
        return;

    char csv_del = ';';
    fprintf(csv_file, "%s", table->row_label_label.text);
    for (uint32 col_i = 0; col_i < table->column_count; col_i++)
        fprintf(csv_file, "%c%s", csv_del, table->column_labels[col_i].text);
    fprintf(csv_file, "\n");

    for (uint32 row_i = 0; row_i < table->row_count; row_i++)
    {
        fprintf(csv_file, "%s", table->row_labels[row_i].text);
        for (uint32 col_i = 0; col_i < table->column_count; col_i++)
        {
            fprintf(csv_file, "%c", csv_del);
            SHM_RepetitionTestResults results = table->results[(row_i * table->column_count) + col_i];
            uint64 avg_tsc = results.total[SHM_RepetitionTestResultType_TSC] / results.run_count;
            uint64 avg_bytes = results.total[SHM_RepetitionTestResultType_BytesProcessed] / results.run_count;
            uint64 avg_p_faults = results.total[SHM_RepetitionTestResultType_PageFaultCount] / results.run_count;
            float64 seconds = _tsc_to_seconds(tester->time_counter_frequency, avg_tsc);
            switch (print_value_type)
            {
                case SHM_RepetitionTestPrintValueType_GBPerSecond:
                {
                    fprintf(csv_file, "%.5f", seconds == 0.0 ? 0.0 : (float64)(avg_bytes / GIBIBYTE(1)) / seconds);
                    break;
                }
            }
        }
        fprintf(csv_file, "\n");
    }

    if (out_csv_filepath)
        fclose(csv_file);
    else
        fprintf(csv_file, "\n");
}
#endif