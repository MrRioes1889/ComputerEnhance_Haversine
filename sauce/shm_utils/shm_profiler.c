#include "shm_profiler.h"

#if SHM_PROFILER_TIMERS_ENABLED
#include "platform/shm_intrin.h"

#ifndef SHM_TIMER_PRINT
#include <stdio.h>
#define SHM_TIMER_PRINT printf_s 
#endif

typedef struct
{
    uint64 tsc_elapsed_self;
    uint64 tsc_elapsed_full;
    uint64 byte_count_processed;
    uint32 call_count;
    const char* key;
}
SHM_Timer;

typedef struct 
{
    uint64 time_counter_frequency;
    uint32 timers_count;
    uint64 tsc_start;
    SHM_Timer timers[SHM_PROFILER_MAX_TIMERS + 1];
}
SHM_Profiler;

static SHM_Profiler _profiler;
static uint32 _cur_timer_id;

void shm_profiler_init(uint64 time_counter_frequency)
{
    _cur_timer_id = 0;
    _profiler = (SHM_Profiler){0};
    _profiler.time_counter_frequency = time_counter_frequency;
    _profiler.tsc_start = shm_intrin_rdtsc();
}

void shm_profiler_dump()
{
    uint64 tsc_elapsed = shm_intrin_rdtsc() - _profiler.tsc_start;

    SHM_TIMER_PRINT("Profiler dump:\n");
	SHM_TIMER_PRINT("RDTSC frequency: %llu cycles/second\n", _profiler.time_counter_frequency);
	SHM_TIMER_PRINT("Estimated total time/count: %.5f ms / %llu cycles\n", ((float64)(tsc_elapsed)/(float64)_profiler.time_counter_frequency) * 1000, tsc_elapsed);
	SHM_TIMER_PRINT("Timers:\n");

    for (uint32 i = 1; i <= _profiler.timers_count; i++)
    {
        SHM_Timer* timer = _profiler.timers + i;
        float64 self_percent = (float64)timer->tsc_elapsed_self/(float64)tsc_elapsed * 100.0;
        float64 self_seconds = (float64)timer->tsc_elapsed_self/(float64)_profiler.time_counter_frequency;
        float64 full_percent = (float64)timer->tsc_elapsed_full/(float64)tsc_elapsed * 100.0;
        float64 full_seconds = (float64)timer->tsc_elapsed_full/(float64)_profiler.time_counter_frequency;
        SHM_TIMER_PRINT("  %s[%u]:\n    self: %.5f ms, %llu cycles, %.2f%%\n    full: %.5f ms, %llu cycles, %.2f%%\n", 
            timer->key, timer->call_count, self_seconds * 1000.0, timer->tsc_elapsed_self, self_percent, full_seconds * 1000.0, timer->tsc_elapsed_full, full_percent);

        if (timer->byte_count_processed)
        {
            float64 megabyte = 1024.0 * 1024.0;

            float64 processed_mb = timer->byte_count_processed / megabyte;
            float64 gb_per_second = (processed_mb / 1024.0) / full_seconds;
            SHM_TIMER_PRINT("    data throughput: %.5f MB at %.5f GB/second\n", processed_mb, gb_per_second);
        }
    }

    shm_profiler_init(_profiler.time_counter_frequency);
}

uint32 __shm_timer_get_next_timer_id()
{
    return _profiler.timers_count < SHM_PROFILER_MAX_TIMERS ? _profiler.timers_count++ + 1 : SHM_PROFILER_MAX_TIMERS;
}

__SHM_Timeblock __shm_timer_start(const char* key, uint32 timer_id, uint64 byte_count_processed)
{
    SHM_Timer* timer = _profiler.timers + timer_id;
    __SHM_Timeblock timeblock;

    timeblock.parent_id = _cur_timer_id;
    timeblock.timer_id = timer_id;
    timeblock.tsc_elapsed_full_carry_offset = timer->tsc_elapsed_full;
    timer->key = key;
    timer->byte_count_processed += byte_count_processed;
    
    _cur_timer_id = timer_id;
    timeblock.tsc_start = shm_intrin_rdtsc();
    return timeblock;
}

void __shm_timer_end(__SHM_Timeblock timeblock)
{
    uint64 tsc_elapsed = shm_intrin_rdtsc() - timeblock.tsc_start;
    _cur_timer_id = timeblock.parent_id;

    SHM_Timer* timer = _profiler.timers + timeblock.timer_id;
    SHM_Timer* parent = _profiler.timers + timeblock.parent_id; 

    parent->tsc_elapsed_self -= tsc_elapsed;
    timer->tsc_elapsed_self += tsc_elapsed;
    timer->tsc_elapsed_full = timeblock.tsc_elapsed_full_carry_offset + tsc_elapsed;
    timer->call_count++;
}

#endif