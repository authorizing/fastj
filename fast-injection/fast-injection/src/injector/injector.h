#pragma once
#include <Windows.h>
#include <string>

class DLLInjector {
public:
    DLLInjector();
    ~DLLInjector();

    bool inject(DWORD processId, const std::wstring& dllPath);
    
private:
    bool enableDebugPrivilege();
}; 