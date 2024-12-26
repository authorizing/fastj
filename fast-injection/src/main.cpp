#include <iostream>
#include <string>
#include "./injector/injector.h"
#include "./utils/process_utils.h"

void printUsage(const wchar_t* programName) {
    std::wcout << L"Usage: " << programName << L" [options]\n"
               << L"Options:\n"
               << L"  -p, --process <name>    Target process name (default: cs2.exe)\n"
               << L"  -d, --dll <path>        Path to DLL file\n"
               << L"  -w, --wait <seconds>    Wait timeout in seconds (default: infinite)\n"
               << L"  -t, --type <type>       Injection type (default: loadlibrary)\n"
               << L"                          Types: ll, mm\n"
               << L"  -h, --help             Show this help message\n";
}

int wmain(int argc, wchar_t* argv[]) {
    std::wstring processName = L"cs2.exe";
    std::wstring dllPath;
    DWORD timeout = INFINITE;
    InjectionType injectionType = InjectionType::LoadLibrary;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::wstring arg = argv[i];
        if (arg == L"-h" || arg == L"--help") {
            printUsage(argv[0]);
            return 0;
        }
        else if ((arg == L"-p" || arg == L"--process") && i + 1 < argc) {
            processName = argv[++i];
        }
        else if ((arg == L"-d" || arg == L"--dll") && i + 1 < argc) {
            dllPath = argv[++i];
        }
        else if ((arg == L"-w" || arg == L"--wait") && i + 1 < argc) {
            timeout = _wtoi(argv[++i]) * 1000;
        }
        else if ((arg == L"-t" || arg == L"--type") && i + 1 < argc) {
            std::wstring type = argv[++i];
            if (_wcsicmp(type.c_str(), L"ll") == 0) {
                injectionType = InjectionType::LoadLibrary;
            }
            else if (_wcsicmp(type.c_str(), L"mm") == 0) {
                injectionType = InjectionType::ManualMap;
            }
            else {
                std::wcout << L"Error: Unknown injection type: " << type << L"\n";
                printUsage(argv[0]);
                return 1;
            }
        }
    }

    if (dllPath.empty()) {
        std::wcout << L"Error: DLL path is required\n";
        printUsage(argv[0]);
        return 1;
    }

    std::wcout << L"Waiting for process: " << processName << L"...\n";

    DWORD processId = 0;
    if (!ProcessUtils::waitForProcess(processName, timeout, &processId)) {
        std::wcout << L"Timeout waiting for process\n";
        return 1;
    }

    std::wcout << L"Found process (PID: " << processId << L")\n";
    std::wcout << L"Injection type: " 
               << (injectionType == InjectionType::LoadLibrary ? L"LoadLibrary" : L"Manual Map") << L"\n";
    std::wcout << L"Injecting: " << dllPath << L"\n";

    DLLInjector injector;
    if (injector.inject(processId, dllPath, injectionType)) {
        std::wcout << L"Successfully injected DLL!\n";
        return 0;
    }
    else {
        std::wcout << L"Failed to inject DLL!\n";
        return 1;
    }
} 