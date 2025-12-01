#pragma once
#include "defines.h"

#define SHM_TIMER_MAX_TIMERS 1023

void shm_profiler_init(uint64 time_counter_frequency);
void shm_profiler_dump();

uint32 __shm_timer_get_next_timer_id();
void __shm_timer_start(const char* key, uint32 timer_id);
void __shm_timer_end();

#define __CONCAT2(A, B) A##B
#define __CONCAT(A, B) __CONCAT2(A, B)
#define __SHM_TIMER_START2(key, id_var) static uint32 id_var = 0; if(! id_var ) id_var = __shm_timer_get_next_timer_id(); __shm_timer_start(key, id_var)
#define __SHM_TIMER_START(key, tu_id) __SHM_TIMER_START2(key, __CONCAT(__shm_timer_id_, tu_id))
#define SHM_TIMER_START_FUNC() __SHM_TIMER_START(__func__, __COUNTER__)
#define SHM_TIMER_START(key) __SHM_TIMER_START(key, __COUNTER__)
#define SHM_TIMER_STOP() __shm_timer_end()