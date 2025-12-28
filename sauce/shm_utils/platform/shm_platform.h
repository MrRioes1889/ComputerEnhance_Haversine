#pragma once
#include "../shm_utils_common_defines.h"

typedef struct
{
    void* handle;
}
SHM_FileHandle;

bool8 shm_platform_context_init();
bool8 shm_platform_context_init_additional_metrics();
void shm_platform_context_destroy();

void* shm_platform_memory_allocate(uint64 size);
void shm_platform_memory_free(void* data);

bool8 shm_platform_is_console_window_attached();
bool8 shm_platform_console_window_open();
bool8 shm_platform_console_window_close();

uint64 shm_platform_metrics_get_page_fault_count();

uint64 shm_platform_get_os_timer_frequency();
uint64 shm_platform_get_os_timer_count();
uint64 shm_platform_get_cpu_timer_frequency(uint64 calibration_ms);

void shm_platform_sleep_ms(uint32 sleep_ms);
void shm_platform_sleep_until_key_pressed();

uint64 shm_platform_get_filesize(const char* filepath);
SHM_FileHandle shm_platform_file_create(const char* filepath, bool8 overwrite);
SHM_FileHandle shm_platform_file_open(const char* filepath);
void shm_platform_file_close(SHM_FileHandle* file);
uint32 shm_platform_file_read(SHM_FileHandle file, void* dest_buffer, uint32 dest_buffer_size);
void shm_platform_file_append(SHM_FileHandle file, void* buffer, uint32 buffer_size);



