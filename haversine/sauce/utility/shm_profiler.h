#pragma once
#include "defines.h"

#define SHM_PROFILER_TIMERS_ENABLED 1
#define SHM_PROFILER_MAX_TIMERS 1023

typedef struct 
{
    uint32 timer_id;
    uint32 parent_id;
    uint64 tsc_elapsed_full_carry_offset;
    uint64 tsc_start;
}
__SHM_Timeblock;


void shm_profiler_init(uint64 time_counter_frequency);
void shm_profiler_dump();

uint32 __shm_timer_get_next_timer_id();
__SHM_Timeblock __shm_timer_start(const char* key, uint32 timer_id, uint64 byte_count_processed);
void __shm_timer_end(__SHM_Timeblock timeblock);

#if SHM_PROFILER_TIMERS_ENABLED
#define __CONCAT2(A, B) A##B
#define __CONCAT(A, B) __CONCAT2(A, B)
#define __SHM_TIMER_START(timeblock_name, key, id_var, byte_count) static uint32 id_var = 0; if(!id_var) id_var = __shm_timer_get_next_timer_id(); __SHM_Timeblock __shm_timeblock_##timeblock_name = __shm_timer_start(key, id_var, byte_count)
#define SHM_TIMER_START_FUNC(timeblock_name) __SHM_TIMER_START(timeblock_name, __func__, __CONCAT(__shm_timer_id_, __COUNTER__), 0)
#define SHM_TIMER_START(timeblock_name, key) __SHM_TIMER_START(timeblock_name, key, __CONCAT(__shm_timer_id_, __COUNTER__), 0)
#define SHM_TIMER_START_FUNC_DATA(timeblock_name, byte_count) __SHM_TIMER_START(timeblock_name, __func__, __CONCAT(__shm_timer_id_, __COUNTER__), byte_count)
#define SHM_TIMER_START_DATA(timeblock_name, key, byte_count) __SHM_TIMER_START(timeblock_name, key, __CONCAT(__shm_timer_id_, __COUNTER__), byte_count)
#define SHM_TIMER_STOP(timeblock_name) __shm_timer_end(__shm_timeblock_##timeblock_name)
#else
#define SHM_TIMER_START_FUNC(...)
#define SHM_TIMER_START(...)
#define SHM_TIMER_STOP(...)
#endif