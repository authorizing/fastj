#pragma once
#include <Windows.h>
#include <string>

enum class InjectionType {
    LoadLibrary,
    ManualMap
};

class DLLInjector {
public:
    DLLInjector();
    ~DLLInjector();

    bool inject(DWORD processId, const std::wstring& dllPath, InjectionType type = InjectionType::LoadLibrary);
    
private:
    bool enableDebugPrivilege();
    bool loadLibraryInject(HANDLE hProcess, const std::wstring& dllPath);
    bool manualMapInject(HANDLE hProcess, const std::wstring& dllPath);
}; 