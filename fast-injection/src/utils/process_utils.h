#pragma once
#include <Windows.h>
#include <string>

class ProcessUtils {
public:
    static DWORD getProcessIdByName(const std::wstring& processName);
    static bool waitForProcess(const std::wstring& processName, DWORD timeout_ms, DWORD* processId);
}; 