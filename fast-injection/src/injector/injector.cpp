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

bool DLLInjector::inject(DWORD processId, const std::wstring& dllPath, InjectionType type) {
    if (!enableDebugPrivilege()) {
        std::wcerr << L"Failed to enable debug privilege" << std::endl;
        return false;
    }

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (hProcess == NULL) {
        std::wcerr << L"Failed to open process" << std::endl;
        return false;
    }

    bool result = false;
    switch (type) {
        case InjectionType::LoadLibrary:
            result = loadLibraryInject(hProcess, dllPath);
            break;
        case InjectionType::ManualMap:
            result = manualMapInject(hProcess, dllPath);
            break;
    }

    CloseHandle(hProcess);
    return result;
}

bool DLLInjector::loadLibraryInject(HANDLE hProcess, const std::wstring& dllPath) {
    size_t dllPathSize = (dllPath.length() + 1) * sizeof(wchar_t);
    LPVOID remoteMem = VirtualAllocEx(hProcess, NULL, dllPathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (remoteMem == NULL) {
        return false;
    }

    if (!WriteProcessMemory(hProcess, remoteMem, dllPath.c_str(), dllPathSize, NULL)) {
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        return false;
    }

    HMODULE kernel32 = GetModuleHandle(L"kernel32.dll");
    LPTHREAD_START_ROUTINE loadLibraryW = (LPTHREAD_START_ROUTINE)GetProcAddress(kernel32, "LoadLibraryW");

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, loadLibraryW, remoteMem, 0, NULL);
    if (hThread == NULL) {
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);

    DWORD exitCode;
    GetExitCodeThread(hThread, &exitCode);

    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);

    return exitCode != 0;
}

bool DLLInjector::manualMapInject(HANDLE hProcess, const std::wstring& dllPath) {
    // Read the DLL file
    HANDLE hFile = CreateFileW(dllPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE) {
        CloseHandle(hFile);
        return false;
    }

    BYTE* fileBuffer = new BYTE[fileSize];
    DWORD bytesRead;
    if (!ReadFile(hFile, fileBuffer, fileSize, &bytesRead, NULL)) {
        delete[] fileBuffer;
        CloseHandle(hFile);
        return false;
    }
    CloseHandle(hFile);

    // Verify DOS and NT headers
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)fileBuffer;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        delete[] fileBuffer;
        return false;
    }

    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(fileBuffer + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
        delete[] fileBuffer;
        return false;
    }

    // Allocate memory in target process
    LPVOID imageBase = VirtualAllocEx(hProcess, NULL, ntHeaders->OptionalHeader.SizeOfImage,
        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!imageBase) {
        delete[] fileBuffer;
        return false;
    }

    // Copy headers
    if (!WriteProcessMemory(hProcess, imageBase, fileBuffer, ntHeaders->OptionalHeader.SizeOfHeaders, NULL)) {
        VirtualFreeEx(hProcess, imageBase, 0, MEM_RELEASE);
        delete[] fileBuffer;
        return false;
    }

    // Copy sections
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(ntHeaders);
    for (UINT i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++, section++) {
        if (section->SizeOfRawData) {
            LPVOID sectionDest = (LPVOID)((LPBYTE)imageBase + section->VirtualAddress);
            LPVOID sectionSrc = (LPVOID)(fileBuffer + section->PointerToRawData);
            if (!WriteProcessMemory(hProcess, sectionDest, sectionSrc, section->SizeOfRawData, NULL)) {
                VirtualFreeEx(hProcess, imageBase, 0, MEM_RELEASE);
                delete[] fileBuffer;
                return false;
            }
        }
    }

    // Handle relocations
    DWORD_PTR deltaBase = (DWORD_PTR)imageBase - ntHeaders->OptionalHeader.ImageBase;
    if (deltaBase) {
        PIMAGE_DATA_DIRECTORY relocDir = &ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        if (relocDir->Size > 0) {
            PIMAGE_BASE_RELOCATION reloc = (PIMAGE_BASE_RELOCATION)(fileBuffer + relocDir->VirtualAddress);
            while (reloc->VirtualAddress) {
                DWORD relocCount = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
                WORD* relocData = (WORD*)((LPBYTE)reloc + sizeof(IMAGE_BASE_RELOCATION));

                for (DWORD i = 0; i < relocCount; i++) {
                    if (relocData[i] >> 12 == IMAGE_REL_BASED_HIGHLOW) {
                        DWORD_PTR* relocAddr = (DWORD_PTR*)((LPBYTE)imageBase + reloc->VirtualAddress + (relocData[i] & 0xFFF));
                        DWORD_PTR relocatedAddr = *relocAddr + deltaBase;
                        WriteProcessMemory(hProcess, relocAddr, &relocatedAddr, sizeof(DWORD_PTR), NULL);
                    }
                }
                reloc = (PIMAGE_BASE_RELOCATION)((LPBYTE)reloc + reloc->SizeOfBlock);
            }
        }
    }

    // Get entry point and create remote thread
    LPTHREAD_START_ROUTINE entryPoint = (LPTHREAD_START_ROUTINE)((LPBYTE)imageBase + ntHeaders->OptionalHeader.AddressOfEntryPoint);
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, entryPoint, NULL, 0, NULL);
    if (!hThread) {
        VirtualFreeEx(hProcess, imageBase, 0, MEM_RELEASE);
        delete[] fileBuffer;
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    delete[] fileBuffer;

    return true;
}