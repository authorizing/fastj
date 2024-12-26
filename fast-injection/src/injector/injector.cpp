#include "injector.h"
#include <iostream>
#include <TlHelp32.h>

DLLInjector::DLLInjector() {}
DLLInjector::~DLLInjector() {}

bool DLLInjector::enableDebugPrivilege() {
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return false;
    }

    LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tkp.Privileges[0].Luid);
    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0)) {
        CloseHandle(hToken);
        return false;
    }

    CloseHandle(hToken);
    return true;
}

bool DLLInjector::inject(DWORD processId, const std::wstring& dllPath) {
    if (!enableDebugPrivilege()) {
        std::wcerr << L"Failed to enable debug privilege" << std::endl;
        return false;
    }

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (hProcess == NULL) {
        std::wcerr << L"Failed to open process" << std::endl;
        return false;
    }

    size_t dllPathSize = (dllPath.length() + 1) * sizeof(wchar_t);
    LPVOID remoteMem = VirtualAllocEx(hProcess, NULL, dllPathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (remoteMem == NULL) {
        CloseHandle(hProcess);
        std::wcerr << L"Failed to allocate memory in target process" << std::endl;
        return false;
    }

    if (!WriteProcessMemory(hProcess, remoteMem, dllPath.c_str(), dllPathSize, NULL)) {
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        std::wcerr << L"Failed to write to process memory" << std::endl;
        return false;
    }

    HMODULE kernel32 = GetModuleHandle(L"kernel32.dll");
    LPTHREAD_START_ROUTINE loadLibraryW = (LPTHREAD_START_ROUTINE)GetProcAddress(kernel32, "LoadLibraryW");

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, loadLibraryW, remoteMem, 0, NULL);
    if (hThread == NULL) {
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        std::wcerr << L"Failed to create remote thread" << std::endl;
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);

    DWORD exitCode;
    GetExitCodeThread(hThread, &exitCode);

    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    return exitCode != 0;
}