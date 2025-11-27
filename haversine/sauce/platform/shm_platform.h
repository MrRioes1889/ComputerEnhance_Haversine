#pragma once
#include "defines.h"

typedef struct
{
    void* proc_handle;
    char executable_dir[256];
    char executable_name[64];
}
SHM_PlatformContext;

typedef struct
{
    void* handle;
}
SHM_FileHandle;

bool8 shm_platform_context_init(SHM_PlatformContext* out_context);

bool8 shm_platform_console_window_open();
bool8 shm_platform_console_window_close();

uint64 shm_platform_get_os_time_counter_frequency();
uint64 shm_platform_get_os_time_counter();
uint64 shm_platform_get_rdtsc_frequency(uint64 calibration_ms);

void shm_platform_sleep_until_key_pressed();

int64 shm_platform_get_filesize(const char* filepath);
SHM_FileHandle shm_platform_file_create(const char* filepath, bool8 overwrite);
SHM_FileHandle shm_platform_file_open(const char* filepath);
void shm_platform_file_close(SHM_FileHandle* file);
uint32 shm_platform_file_read(SHM_FileHandle file, void* dest_buffer, uint32 dest_buffer_size);
void shm_platform_file_append(SHM_FileHandle file, void* buffer, uint32 buffer_size);



