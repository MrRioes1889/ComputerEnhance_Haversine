#include "shm_profiler.h"
#include "platform/shm_intrin.h"

#ifndef SHM_TIMER_PRINT
#include <stdio.h>
#define SHM_TIMER_PRINT printf_s 
#endif

typedef struct
{
    const char* key;
    uint64 tsc_elapsed_children_included;
    uint64 tsc_elapsed_children_excluded;
    uint64 tsc_last_start;
    uint32 parent_id;
    uint32 hit_count;
}
SHM_Timer;

typedef struct 
{
    uint64 time_counter_frequency;
    uint32 timers_count;
    uint32 timers_running_count;
    SHM_Timer timers[SHM_TIMER_MAX_TIMERS + 1];
    bool8 timers_running_flags[SHM_TIMER_MAX_TIMERS + 1];
}
SHM_Profiler;

static SHM_Profiler _profiler;
static uint32 _cur_parent_id;

void shm_profiler_init(uint64 time_counter_frequency)
{
    _profiler = (SHM_Profiler){0};
    _profiler.time_counter_frequency = time_counter_frequency;
    __shm_timer_start("Profiler", 0);
}

void shm_profiler_dump()
{
    SHM_Timer* timer = _profiler.timers + _cur_parent_id; 
    uint64 tsc_elapsed = shm_intrin_rdtsc() - timer->tsc_last_start;

    SHM_TIMER_PRINT("Profiler dump:\n");
	SHM_TIMER_PRINT("RDTSC frequency: %llu cycles/second\n", _profiler.time_counter_frequency);
	SHM_TIMER_PRINT("Estimated total time/count: %.5f ms / %llu cycles\n", ((float64)(tsc_elapsed)/(float64)_profiler.time_counter_frequency) * 1000, tsc_elapsed);
	SHM_TIMER_PRINT("Timers:\n");

    for (uint32 i = 1; i <= _profiler.timers_count; i++)
    {
        SHM_Timer* timer = _profiler.timers + i;
        float64 wo_children_p = (float64)timer->tsc_elapsed_children_excluded/(float64)tsc_elapsed * 100.0;
        float64 wo_children_ms = (float64)timer->tsc_elapsed_children_excluded/(float64)_profiler.time_counter_frequency * 1000;
        float64 with_children_p = (float64)timer->tsc_elapsed_children_included/(float64)tsc_elapsed * 100.0;
        float64 with_children_ms = (float64)timer->tsc_elapsed_children_included/(float64)_profiler.time_counter_frequency * 1000;
        float exclusive_p = 
        SHM_TIMER_PRINT("  %s:\n    w/o children:  %.5f ms, %llu cycles, %.2f%%\n    with children: %.5f ms, %llu cycles, %.2f%%\n", 
            timer->key, wo_children_ms, timer->tsc_elapsed_children_excluded, wo_children_p, with_children_ms, timer->tsc_elapsed_children_included, with_children_p);
    }

    shm_profiler_init(_profiler.time_counter_frequency);
}

uint32 __shm_timer_get_next_timer_id()
{
    return _profiler.timers_count < SHM_TIMER_MAX_TIMERS ? _profiler.timers_count++ + 1 : SHM_TIMER_MAX_TIMERS;
}

void __shm_timer_start(const char* key, uint32 timer_id)
{
    SHM_Timer* timer = _profiler.timers + timer_id; 
    if (_profiler.timers_running_flags[timer_id])
        return;

    timer->parent_id = _cur_parent_id;
    _cur_parent_id = timer_id;
    timer->key = key;
    timer->tsc_last_start = shm_intrin_rdtsc();
    _profiler.timers_running_flags[timer_id] = true;
    _profiler.timers_running_count++;
}

void __shm_timer_end()
{
    if (!_cur_parent_id)
        return;

    SHM_Timer* timer = _profiler.timers + _cur_parent_id; 
    SHM_Timer* parent = _profiler.timers + timer->parent_id; 
    _profiler.timers_running_flags[_cur_parent_id] = false;
    _cur_parent_id = timer->parent_id;

    uint64 tsc_elapsed = shm_intrin_rdtsc() - timer->tsc_last_start;
    parent->tsc_elapsed_children_excluded -= tsc_elapsed;
    timer->tsc_elapsed_children_excluded += tsc_elapsed;
    timer->tsc_elapsed_children_included += tsc_elapsed;
    timer->hit_count++;

    _profiler.timers_running_count--;
}