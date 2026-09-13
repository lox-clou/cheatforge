#include <windows.h>
#include <iostream>
#include <tlhelp32.h>
#include <string>
#include <vector>

using namespace std;

// Игра: cs2.exe
// Процесс: cs2.exe
// Модуль: client.dll

DWORD GetProcessId(const wstring& name) {
    DWORD pid = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            if (name == pe.szExeFile) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

uintptr_t GetModuleBase(DWORD pid, const wstring& modName) {
    uintptr_t base = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    MODULEENTRY32W me;
    me.dwSize = sizeof(me);
    if (Module32FirstW(snap, &me)) {
        do {
            if (modName == me.szModule) {
                base = (uintptr_t)me.modBaseAddr;
                break;
            }
        } while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
    return base;
}

int main() {
    wstring processName = L"cs2.exe";
    wstring moduleName = L"client.dll";
    
    cout << "=== CheatForge: cs2.exe ===" << endl;
    cout << "Waiting for " << processName << "..." << endl;
    
    DWORD pid = 0;
    while (pid == 0) {
        pid = GetProcessId(processName);
        Sleep(1000);
    }
    
    cout << "[+] Found process PID: " << pid << endl;
    
    uintptr_t base = GetModuleBase(pid, moduleName);
    if (base == 0) {
        cerr << "[-] Module not found!" << endl;
        system("pause");
        return 1;
    }
    
    cout << "[+] Module base: 0x" << hex << base << dec << endl;
    cout << "[+] Features: aimbot, triggerbot, rcs, esp, glow, bhop, radar, skinchanger" << endl;
    cout << "[+] Press ESC to exit" << endl;
    
    // Main loop
    while (!GetAsyncKeyState(VK_ESCAPE)) {
        // Feature implementations here
        Sleep(10);
    }
    
    cout << "[+] Exit" << endl;
    return 0;
}
