# DLL Injector

A simple, modern C++ DLL injector with command-line interface. This project demonstrates DLL injection using both LoadLibrary and Manual Mapping methods.

## Features

- 🚀 Clean, modern C++ implementation
- 💉 Multiple injection methods:
  - LoadLibrary injection
  - Manual Mapping injection
- 🎯 Process targeting by name
- ⏱️ Configurable process wait timeout
- 🛠️ Command-line interface

## Building

### Prerequisites

- Visual Studio 2019 or higher with C++ Desktop development workload
- Windows SDK

### Installation

1. Open the solution in Visual Studio
2. Build in Release mode
3. Add the output directory to your system PATH:

#### Adding to PATH (choose one method):

**Using Windows Settings (Recommended):**
1. Press `Win + X` and select "System"
2. Click on "Advanced system settings"
3. Click "Environment Variables"
4. Under "User variables", find and select "Path"
5. Click "New"
6. Add the path to your dll-injector binary (e.g., `C:\Path\To\Your\Release\Folder`)
7. Click "OK" on all windows
8. Restart any open terminal windows

**Using PowerShell (Run as Administrator):**

```powershell
[Environment]::SetEnvironmentVariable(
"Path",
[Environment]::GetEnvironmentVariable("Path", "User") + ";C:\Path\To\Your\Release\Folder",
"User"
)
```

**Using Command Prompt (Run as Administrator):**

```cmd
setx PATH "%PATH%;C:\Path\To\Your\Release\Folder"
```


## Usage

Basic syntax:

```
dll-injector [options]
Options:
-p, --process <name> Target process name (default: cs2.exe)
-d, --dll <path> Path to DLL file
-w, --wait <seconds> Wait timeout in seconds (default: infinite)
-t, --type <type> Injection type (default: loadlibrary)
Types: loadlibrary, manualmap
-h, --help Show this help message
```

Examples:

```bash
LoadLibrary injection into specific process
dll-injector -p notepad.exe -d path/to/your.dll -t loadlibrary
Manual Map injection with 30 second wait
dll-injector -p cs2.exe -d path/to/your.dll -t manualmap -w 30
Show help
dll-injector --help
```

## Injection Methods

### LoadLibrary Injection
The classic method that uses Windows' LoadLibrary function. Simple but easily detectable.
- Pros:
  - Simple implementation
  - Reliable
  - Handles DLL dependencies automatically
- Cons:
  - Easily detectable
  - Shows up in loaded modules list
  - Limited control over loading process

### Manual Mapping
A more advanced method that manually maps the PE file into memory.
- Pros:
  - More stealthy
  - Doesn't use LoadLibrary
  - Not visible in loaded modules list
  - Full control over loading process
- Cons:
  - More complex implementation
  - Must handle relocations manually
  - May fail with complex DLLs
  - Dependencies must be handled manually

## Security Considerations

This tool is for educational purposes only. Be aware that:
- DLL injection can be detected by anti-cheat systems
- Some injection methods may be more detectable than others
- Always test in a safe environment
- Use at your own risk

## Contributing

Contributions are welcome! Feel free to:
- Add new injection methods
- Improve error handling
- Enhance detection avoidance
- Add features
- Fix bugs

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Disclaimer

This project is for educational purposes only. The authors are not responsible for any misuse or damage caused by this program. Users are responsible for ensuring they have permission to inject DLLs into target processes.
