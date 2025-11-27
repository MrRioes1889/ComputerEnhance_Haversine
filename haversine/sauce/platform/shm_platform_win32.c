#ifdef _WIN32

#include "shm_platform.h"
#include "shm_intrin.h"
#include "utility/shm_string.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

bool8 shm_platform_context_init(SHM_PlatformContext* out_context)
{
    out_context->proc_handle = GetModuleHandle(0);

    char exec_filepath[PATH_MAX] = {0};
    uint32 filepath_length = GetModuleFileNameA((HMODULE)out_context->proc_handle, exec_filepath, array_count(exec_filepath));
    int32 split_i = shm_cstring_last_index_of(exec_filepath, '\\');
    shm_cstring_copy_n(out_context->executable_dir, array_count(out_context->executable_dir), exec_filepath, (uint32)split_i);
    shm_cstring_copy(out_context->executable_name, array_count(out_context->executable_name), &exec_filepath[split_i+1]);

    return true;
}

uint64 shm_platform_get_os_time_counter_frequency()
{
    LARGE_INTEGER qpf;
    QueryPerformanceFrequency(&qpf);
    return qpf.QuadPart;
}

uint64 shm_platform_get_os_time_counter()
{
    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);
    return qpc.QuadPart;
}

uint64 shm_platform_get_rdtsc_frequency(uint64 calibration_ms)
{
    if (!calibration_ms)
        calibration_ms = 1000;
    uint64 os_freq = shm_platform_get_os_time_counter_frequency();

    uint64 rdtsc_start = __rdtsc();
	uint64 os_start = shm_platform_get_os_time_counter();
	uint64 os_end = 0;
	uint64 os_elapsed = 0;
    uint64 os_wait = os_freq * calibration_ms / 1000;
	while(os_elapsed < os_wait)
	{
		os_end = shm_platform_get_os_time_counter();
		os_elapsed = os_end - os_start;
	}

    uint64 rdtsc_end = __rdtsc();
    uint64 rdtsc_elapsed = rdtsc_end - rdtsc_start;
    uint64 rdtsc_freq = os_freq * rdtsc_elapsed / os_elapsed;
	
    return rdtsc_freq;
}

void shm_platform_sleep_until_key_pressed()
{
    HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
    if (handle == INVALID_HANDLE_VALUE)
        return;

    DWORD count = 0;
    INPUT_RECORD input_record;

    for (;;)
    {
        DWORD wait = WaitForSingleObject(handle, INFINITE);
        if (wait != WAIT_OBJECT_0)
            return;

        if (!ReadConsoleInput(handle, &input_record, 1, &count))
            return;

        if (input_record.EventType == KEY_EVENT && input_record.Event.KeyEvent.bKeyDown)
            return;
    }
}

bool8 shm_platform_console_window_open()
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

bool8 shm_platform_console_window_close()
{
    FreeConsole();
    return true;
}

int64 shm_platform_get_filesize(const char* filepath)
{
    HANDLE file_handle = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file_handle == INVALID_HANDLE_VALUE)
        return 0;

    DWORD high = 0;
    DWORD low = GetFileSize(file_handle, &high);
    CloseHandle(file_handle);
    LARGE_INTEGER ret;
    ret.LowPart = low;
    ret.HighPart = high;
    return ret.QuadPart;
}

SHM_FileHandle shm_platform_file_create(const char* filepath, bool8 overwrite)
{
    SHM_FileHandle file = {0};
    file.handle = CreateFileA(filepath, GENERIC_READ | GENERIC_WRITE, 0, 0, overwrite ? CREATE_ALWAYS : CREATE_NEW, 0, 0);
    return file;
}

SHM_FileHandle shm_platform_file_open(const char* filepath)
{
    SHM_FileHandle file = {0};
    file.handle = CreateFileA(filepath, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    return file;
}

void shm_platform_file_close(SHM_FileHandle* file)
{
    CloseHandle(file->handle);
    file->handle = 0;
}

uint32 shm_platform_file_read(SHM_FileHandle file, void* dest_buffer, uint32 dest_buffer_size)
{
    DWORD bytes_read = 0;
    ReadFile(file.handle, dest_buffer, dest_buffer_size, &bytes_read, 0);
    return bytes_read;
}

void shm_platform_file_append(SHM_FileHandle file, void* buffer, uint32 buffer_size)
{
    DWORD bytes_written = 0;
    OVERLAPPED overlapped = {0};
    overlapped.Offset = 0xFFFFFFFF;
    overlapped.OffsetHigh = 0xFFFFFFFF;
    WriteFile(file.handle, buffer, buffer_size, &bytes_written, &overlapped);
}

#endif