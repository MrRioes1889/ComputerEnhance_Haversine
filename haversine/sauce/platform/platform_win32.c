#ifdef _WIN32

#include "platform.h"
#include "utility/shm_string.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

bool8 platform_context_init(SHM_PlatformContext* out_context)
{
    out_context->proc_handle = GetModuleHandle(0);

    char exec_filepath[PATH_MAX] = {0};
    uint32 filepath_length = GetModuleFileNameA((HMODULE)out_context->proc_handle, exec_filepath, array_count(exec_filepath));
    int32 split_i = cstring_last_index_of(exec_filepath, '\\');
    cstring_copy_n(out_context->executable_dir, array_count(out_context->executable_dir), exec_filepath, (uint32)split_i);
    cstring_copy(out_context->executable_name, array_count(out_context->executable_name), &exec_filepath[split_i+1]);
    return true;
}

bool8 platform_console_window_open()
{
    FreeConsole();
    WINBOOL res = AllocConsole();
    if (!res)
    {
        DWORD err_code = GetLastError();
        MessageBoxA(0, "Error: Could not open console window!", "Error", MB_OK);
        return false;
    }

    freopen64("CONOUT$", "w", stdout);
    freopen64("CONOUT$", "w", stderr);
    freopen64("CONIN$", "r", stdin);
    return true;
}

bool8 platform_console_window_close()
{
    FreeConsole();
    return true;
}

SHM_FileHandle platform_file_create(const char* filepath, bool8 overwrite)
{
    SHM_FileHandle file = {0};
    file.handle = CreateFileA(filepath, GENERIC_READ | GENERIC_WRITE, 0, 0, overwrite ? CREATE_ALWAYS : CREATE_NEW, 0, 0);
    return file;
}

void platform_file_close(SHM_FileHandle* file)
{
    CloseHandle(file->handle);
    file->handle = 0;
}

void platform_file_append(SHM_FileHandle file, void* buffer, uint32 buffer_size)
{
    DWORD bytes_written = 0;
    OVERLAPPED overlapped = {0};
    overlapped.Offset = 0xFFFFFFFF;
    overlapped.OffsetHigh = 0xFFFFFFFF;
    WriteFile(file.handle, buffer, buffer_size, &bytes_written, &overlapped);
}

#endif