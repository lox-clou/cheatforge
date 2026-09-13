#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <cstdlib>

using namespace std;

struct GameConfig {
    string processName;
    string moduleName;
    map<string, uintptr_t> offsets;
    vector<string> features;
};

map<string, GameConfig> games = {
    {"cs2", {"cs2.exe", "client.dll", {{"entityList", 0x18C4F28}, {"localPlayer", 0x17C66B8}, {"health", 0x100}, {"team", 0x3E}}, {"aimbot","esp","bhop","rcs","triggerbot","glow","radar","skinchanger"}}},
    {"valorant", {"VALORANT.exe", "VALORANT.exe", {{"entityList", 0x7823F40}, {"localPlayer", 0x7823E80}, {"health", 0x1C0}, {"team", 0x108}}, {"aimbot","esp","bhop","radar","triggerbot","glow"}}},
    {"rust", {"RustClient.exe", "GameAssembly.dll", {{"entityList", 0x1A2B3C4}, {"localPlayer", 0x1A2B3C0}, {"health", 0x200}, {"team", 0x210}}, {"silent_aim","player_esp","ore_esp","fly","speed","nofall","autofarm"}}},
    {"apex", {"r5apex.exe", "r5apex.exe", {{"entityList", 0x192D4F0}, {"localPlayer", 0x192D4E0}, {"health", 0x420}, {"team", 0x430}}, {"aim_assist","glow_esp","item_esp","tapstrafe","superglide","no_recoil"}}},
    {"gta5", {"GTA5.exe", "GTA5.exe", {{"localPlayer", 0x2345670}, {"health", 0x280}, {"armor", 0x284}}, {"godmode","infinite_ammo","teleport","noclip","super_jump","vehicle_esp","money"}}},
    {"minecraft", {"javaw.exe", "minecraft.jar", {{"localPlayer", 0x0}, {"health", 0x0}}, {"killaura","reach","fly","speed","xray","esp","autotool","scaffold"}}},
    {"tf2", {"hl2.exe", "client.dll", {{"entityList", 0x1234567}, {"localPlayer", 0x1234560}, {"health", 0xFC}}, {"aimbot","esp","bhop","rocketjump","airstrafe","backstab"}}}
};

DWORD GetPID(const string& name) {
    DWORD pid = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);
    if (Process32First(snap, &pe)) {
        do {
            if (name == pe.szExeFile) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

uintptr_t GetModuleBase(DWORD pid, const string& modName) {
    uintptr_t base = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    MODULEENTRY32 me;
    me.dwSize = sizeof(me);
    if (Module32First(snap, &me)) {
        do {
            if (modName == me.szModule) {
                base = (uintptr_t)me.modBaseAddr;
                break;
            }
        } while (Module32Next(snap, &me));
    }
    CloseHandle(snap);
    return base;
}

int main(int argc, char* argv[]) {
    string gameKey = argc > 1 ? argv[1] : "cs2";
    
    if (games.find(gameKey) == games.end()) {
        cerr << "Unknown game: " << gameKey << endl;
        cerr << "Available: cs2, valorant, rust, apex, gta5, minecraft, tf2" << endl;
        return 1;
    }
    
    auto& cfg = games[gameKey];
    
    cout << "=== CheatForge: " << gameKey << " ===" << endl;
    cout << "Process: " << cfg.processName << endl;
    cout << "Module: " << cfg.moduleName << endl;
    cout << "Features: ";
    for (auto& f : cfg.features) cout << f << " ";
    cout << endl;
    cout << "Waiting for process..." << endl;
    
    DWORD pid = 0;
    while (pid == 0) {
        pid = GetPID(cfg.processName);
        Sleep(1000);
    }
    
    cout << "[+] Found PID: " << pid << endl;
    
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProc) {
        cerr << "[-] Failed to open process. Run as Administrator!" << endl;
        return 1;
    }
    
    uintptr_t base = GetModuleBase(pid, cfg.moduleName);
    if (base == 0) {
        cerr << "[-] Module not found!" << endl;
        CloseHandle(hProc);
        return 1;
    }
    
    cout << "[+] Module base: 0x" << hex << base << dec << endl;
    cout << "[+] Cheat active! Press ESC to exit." << endl;
    
    // Main loop
    while (!GetAsyncKeyState(VK_ESCAPE)) {
        // Read health as demo
        if (cfg.offsets.count("localPlayer") && cfg.offsets["localPlayer"] > 0) {
            uintptr_t localPtr;
            if (ReadProcessMemory(hProc, (LPCVOID)(base + cfg.offsets["localPlayer"]), &localPtr, sizeof(localPtr), nullptr) && localPtr != 0) {
                int hp = 0;
                if (ReadProcessMemory(hProc, (LPCVOID)(localPtr + cfg.offsets["health"]), &hp, sizeof(hp), nullptr)) {
                    cout << "\r[HP: " << hp << "]    " << flush;
                }
            }
        }
        Sleep(100);
    }
    
    CloseHandle(hProc);
    cout << "\n[+] Exit." << endl;
    return 0;
}
