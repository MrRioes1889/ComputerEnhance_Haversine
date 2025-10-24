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


bool8 platform_context_init(SHM_PlatformContext* out_context);

bool8 platform_console_window_open();
bool8 platform_console_window_close();

SHM_FileHandle platform_file_create(const char* filepath, bool8 overwrite);
void platform_file_close(SHM_FileHandle* file);
void platform_file_append(SHM_FileHandle file, void* buffer, uint32 buffer_size);

