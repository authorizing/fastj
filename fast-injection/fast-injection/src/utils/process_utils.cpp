#include "./process_utils.h"
#include <TlHelp32.h>
#include <chrono>

DWORD ProcessUtils::getProcessIdByName(const std::wstring& processName) {
    DWORD processId = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W processEntry;
        processEntry.dwSize = sizeof(processEntry);
        
        if (Process32FirstW(snapshot, &processEntry)) {
            do {
                if (_wcsicmp(processEntry.szExeFile, processName.c_str()) == 0) {
                    processId = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &processEntry));
        }
        
        CloseHandle(snapshot);
    }
    
    return processId;
}

bool ProcessUtils::waitForProcess(const std::wstring& processName, DWORD timeout_ms, DWORD* processId) {
    auto start = std::chrono::steady_clock::now();
    
    while (true) {
        *processId = getProcessIdByName(processName);
        if (*processId != 0) return true;

        if (timeout_ms != INFINITE) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
            if (elapsed >= timeout_ms) return false;
        }
        
        Sleep(100);
    }
} 