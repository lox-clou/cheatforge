// CheatForge external framework — multi-game, GDI overlay, input-level aim
// usage: CheatForge_<game>.exe [-ghost] [-etw] | config.json next to exe selects features
#include <windows.h>
#include <tlhelp32.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include <random>
#include "offsets_gen.h"

// xor-строки: в бинарнике нет plaintext имён процессов/модулей
template<size_t N> struct XStr { char d[N]; constexpr XStr(const char(&s)[N]) { for (size_t i = 0; i < N; i++) d[i] = s[i] ^ 0x5A; } std::string get() const { std::string r; for (size_t i = 0; i < N - 1; i++) r += char(d[i] ^ 0x5A); return r; } };
#define XS(s) XStr<sizeof(s)>(s).get()

static std::mt19937 rng(std::random_device{}());
static int jit(int a, int b) { return a + (int)(rng() % (uint32_t)(b - a + 1)); }

struct Mem { // external: только RPM из долгоживущего read-хендла
    HANDLE hr = 0; DWORD pid = 0;
    bool attach(const char* proc) {
        HANDLE sn = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32 pe; pe.dwSize = sizeof(pe);
        if (Process32First(sn, &pe)) do { if (!_stricmp(pe.szExeFile, proc)) { pid = pe.th32ProcessID; break; } } while (Process32Next(sn, &pe));
        CloseHandle(sn);
        if (!pid) return false;
        hr = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid); // минимальные права чтения
        return hr != 0;
    }
    uintptr_t modBase(const char* mod) {
        HANDLE sn = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (sn == INVALID_HANDLE_VALUE) return 0;
        MODULEENTRY32 me; me.dwSize = sizeof(me); uintptr_t b = 0;
        if (Module32First(sn, &me)) do { if (!_stricmp(me.szModule, mod)) { b = (uintptr_t)me.modBaseAddr; break; } } while (Module32Next(sn, &me));
        CloseHandle(sn); return b;
    }
    template<class T> T rd(uintptr_t a) { T v = {}; ReadProcessMemory(hr, (LPCVOID)a, &v, sizeof(T), 0); return v; }
    template<class T> bool wr(uintptr_t a, T v) { // write-хендл живёт только на время записи
        HANDLE hw = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pid);
        if (!hw) return false;
        bool ok = WriteProcessMemory(hw, (LPVOID)a, &v, sizeof(T), 0) != 0;
        CloseHandle(hw); return ok;
    }
};

struct Prof { const char* key, * proc, * mod; uintptr_t entList, pawn, viewMat, forceJump; int hp, team, origin, dormant, flags, punch, shots, cross, flash, ang; };
static Prof P0(const char* k, const char* p, const char* m) { Prof r = {}; r.key = k; r.proc = p; r.mod = m; return r; }
static Prof PROFS[] = {
    { "cs2", "cs2.exe", "client.dll", OFF_ENTLIST, OFF_PAWN, OFF_VIEWMAT, OFF_FORCEJUMP, OFF_HP, OFF_TEAM, OFF_ORIGIN, OFF_DORMANT, OFF_FLAGS, OFF_PUNCH, OFF_SHOTS, OFF_CROSS, OFF_FLASH, OFF_ANG },
    P0("tf2", "hl2.exe", "client.dll"),
    P0("valorant", "VALORANT-Win64-Shipping.exe", "VALORANT-Win64-Shipping.exe"),
    P0("rust", "RustClient.exe", "GameAssembly.dll"),
    P0("apex", "r5apex.exe", "r5apex.exe"),
    P0("pubg", "TslGame.exe", "TslGame.exe"),
    P0("fortnite", "FortniteClient-Win64-Shipping.exe", "FortniteClient-Win64-Shipping.exe"),
    P0("gta5", "GTA5.exe", "GTA5.exe"),
    P0("minecraft", "javaw.exe", "javaw.exe"),
};

// ---- оверлей: GDI + colorkey, класс окна с бытовым именем ----
struct DI { int t; int x, y, w, h; DWORD col; char txt[24]; };
static std::vector<DI> g_items; static CRITICAL_SECTION g_cs; static HWND g_ov = 0; static int g_sw, g_sh;
static LRESULT CALLBACK OvProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_PAINT) {
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        RECT r; GetClientRect(h, &r);
        HBRUSH bg = CreateSolidBrush(RGB(255, 0,255)); FillRect(dc, &r, bg); DeleteObject(bg);
        SetBkMode(dc, TRANSPARENT);
        EnterCriticalSection(&g_cs);
        for (auto& it : g_items) {
            if (it.t == 0) { HPEN p = CreatePen(PS_SOLID, 1, it.col); HGDIOBJ op = SelectObject(dc, p); HGDIOBJ ob = SelectObject(dc, GetStockObject(NULL_BRUSH)); Rectangle(dc, it.x, it.y, it.x + it.w, it.y + it.h); SelectObject(dc, op); SelectObject(dc, ob); DeleteObject(p); }
            else if (it.t == 1) { HPEN p = CreatePen(PS_SOLID, 1, it.col); HGDIOBJ op = SelectObject(dc, p); MoveToEx(dc, it.x, it.y, 0); LineTo(dc, it.w, it.h); SelectObject(dc, op); DeleteObject(p); }
            else if (it.t == 2) { SetTextColor(dc, it.col); TextOutA(dc, it.x, it.y, it.txt, (int)strlen(it.txt)); }
            else { HBRUSH b = CreateSolidBrush(it.col); RECT rr = { it.x - 2, it.y - 2, it.x + 2, it.y + 2 }; FillRect(dc, &rr, b); DeleteObject(b); }
        }
        LeaveCriticalSection(&g_cs);
        EndPaint(h, &ps); return 0;
    }
    return DefWindowProcA(h, m, w, l);
}
static DWORD WINAPI OvThread(LPVOID) {
    static std::string cls = XS("EdgeUiWindow");
    WNDCLASSA wc = {}; wc.lpfnWndProc = OvProc; wc.hInstance = GetModuleHandleA(0); wc.lpszClassName = cls.c_str();
    RegisterClassA(&wc);
    g_ov = CreateWindowExA(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW, cls.c_str(), "", WS_POPUP | WS_VISIBLE, 0, 0, g_sw, g_sh, 0, 0, wc.hInstance, 0);
    SetLayeredWindowAttributes(g_ov, RGB(255, 0, 255), 0, LWA_COLORKEY);
    MSG msg; while (GetMessageA(&msg, 0, 0, 0)) { TranslateMessage(&msg); DispatchMessageA(&msg); }
    return 0;
}
static bool w2s(const float* vm, float x, float y, float z, float& ox, float& oy) {
    float w = vm[12] * x + vm[13] * y + vm[14] * z + vm[15];
    if (w < 0.01f) return false;
    float sx = vm[0] * x + vm[1] * y + vm[2] * z + vm[3];
    float sy = vm[4] * x + vm[5] * y + vm[6] * z + vm[7];
    ox = (g_sw / 2) * (1 + sx / w); oy = (g_sh / 2) * (1 - sy / w); return true;
}
static void moveMouse(float tx, float ty, float smooth) { // input-level аим: память углов не трогаем
    POINT c; GetCursorPos(&c);
    float nx = c.x + (tx - c.x) / smooth, ny = c.y + (ty - c.y) / smooth;
    INPUT in = {}; in.type = INPUT_MOUSE; in.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
    in.mi.dx = (LONG)((nx / GetSystemMetrics(SM_CXVIRTUALSCREEN)) * 65535);
    in.mi.dy = (LONG)((ny / GetSystemMetrics(SM_CYVIRTUALSCREEN)) * 65535);
    SendInput(1, &in, sizeof(in));
}
static void patchETW() { // глушит usermode ETW-телеметрию
    HMODULE n = GetModuleHandleA("ntdll.dll"); if (!n) return;
    BYTE* p = (BYTE*)GetProcAddress(n, "EtwEventWrite"); if (!p) return;
    DWORD old; VirtualProtect(p, 4, PAGE_EXECUTE_READWRITE, &old);
    p[0] = 0xC3;
    VirtualProtect(p, 4, old, &old);
}
static void ghost(const char* game) { // маскарад: копия под бытовым именем в %TEMP%
    char self[MAX_PATH]; GetModuleFileNameA(0, self, MAX_PATH);
    std::string dst = std::string(getenv("TEMP")) + "\\OneDriveSync.exe";
    CopyFileA(self, dst.c_str(), FALSE);
    ShellExecuteA(0, "open", dst.c_str(), game, "", SW_SHOW);
    MoveFileExA(self, 0, MOVEFILE_DELAY_UNTIL_REBOOT);
    ExitProcess(0);
}
static std::string readAll(const char* p) { FILE* f = fopen(p, "rb"); if (!f) return ""; fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET); std::string s(n, 0); if (n) fread(&s[0], 1, n, f); fclose(f); return s; }
static bool cfgHas(const std::string& s, const char* f) { std::string a = ":"; a += f; a += "\""; std::string b = "\""; b += f; b += "\""; return s.find(a) != std::string::npos || s.find(b) != std::string::npos; }

int main(int argc, char** argv) {
    std::string cfg = readAll("config.json");
    std::string key = "cs2";
    { char self[MAX_PATH]; GetModuleFileNameA(0, self, MAX_PATH); std::string n = strrchr(self, '\\') ? strrchr(self, '\\') + 1 : self;
      for (auto& pr : PROFS) { std::string tk = std::string("CheatForge_") + pr.key; if (n.find(tk) == 0 || n.find(pr.key) != std::string::npos) key = pr.key; } }
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-etw")) patchETW();
        else if (!strcmp(argv[i], "-ghost")) ghost(key.c_str());
        else key = argv[i];
    }
    Prof* pr = 0; for (auto& p : PROFS) if (key == p.key) pr = &p;
    if (!pr) { printf("[cf] unknown game %s\n", key.c_str()); return 1; }
    if (!pr->entList) { printf("[cf] profile '%s': offsets not filled. put them into PROFS[] from a dumper/reclass for that game, rebuild, features light up automatically.\n", pr->key); return 1; }
    bool fEsp = cfg.empty() ? true : cfgHas(cfg, "esp") || cfgHas(cfg, "playeresp");
    bool fAim = cfgHas(cfg, "aimbot") || cfgHas(cfg, "aim_assist");
    bool fTrig = cfgHas(cfg, "triggerbot");
    bool fBhop = cfgHas(cfg, "bhop");
    bool fRcs = cfgHas(cfg, "rcs") || cfgHas(cfg, "norecoil");
    bool fRadar = cfgHas(cfg, "radar");
    bool fFlash = cfgHas(cfg, "remove_flash");
    printf("[cf] game=%s esp=%d aim=%d trig=%d bhop=%d rcs=%d radar=%d noflash=%d\n", pr->key, fEsp, fAim, fTrig, fBhop, fRcs, fRadar, fFlash);
    printf("[cf] waiting for %s ...\n", pr->proc);
    Mem mem; while (!mem.attach(pr->proc)) Sleep(1000);
    uintptr_t base = mem.modBase(pr->mod);
    printf("[cf] pid=%lu base=0x%llx\n", (unsigned long)mem.pid, (unsigned long long)base);
    g_sw = GetSystemMetrics(SM_CXSCREEN); g_sh = GetSystemMetrics(SM_CYSCREEN);
    InitializeCriticalSection(&g_cs);
    CreateThread(0, 0, OvThread, 0, 0, 0);
    printf("[cf] overlay up | END = exit\n");
    float prevPunchX = 0, prevPunchY = 0; int prevShots = 0; bool lastJump = false;
    while (!(GetAsyncKeyState(VK_END) & 0x8000)) {
        std::vector<DI> items;
        float vm[16] = { 0 };
        for (int i = 0; i < 16; i++) vm[i] = mem.rd<float>(base + pr->viewMat + i * 4);
        uintptr_t local = mem.rd<uintptr_t>(base + pr->pawn);
        int myTeam = local ? mem.rd<int>(local + pr->team) : 0;
        float myAng = local && pr->ang ? mem.rd<float>(local + pr->ang + 4) : 0.f;
        if (fRcs && local && pr->punch) { // компенсация отдачи мышью, не памятью
            int shots = pr->shots ? mem.rd<int>(local + pr->shots) : 0;
            float px = mem.rd<float>(local + pr->punch), py = mem.rd<float>(local + pr->punch + 4);
            if (shots > prevShots) {
                INPUT in = {}; in.type = INPUT_MOUSE; in.mi.dwFlags = MOUSEEVENTF_MOVE;
                in.mi.dx = (LONG)-((px - prevPunchX) * 2.0f); in.mi.dy = (LONG)-((py - prevPunchY) * 2.0f);
                SendInput(1, &in, sizeof(in));
            }
            prevShots = shots; prevPunchX = px; prevPunchY = py;
        }
        if (fBhop && local && pr->flags && pr->forceJump) {
            int fl = mem.rd<int>(local + pr->flags);
            bool want = (GetAsyncKeyState(VK_SPACE) & 0x8000) && (fl & 1);
            if (want != lastJump) { mem.wr<int>(base + pr->forceJump, want ? 6 : 0); lastJump = want; }
        }
        if (fFlash && local && pr->flash) mem.wr<float>(local + pr->flash, 0.f);
        float bestD = 1e9f, bestX = 0, bestY = 0; bool have = false;
        for (int i = 0; i < 64; i++) {
            uintptr_t le = mem.rd<uintptr_t>(base + pr->entList + 0x8 * (i >> 9) + 0x10);
            if (!le) continue;
            uintptr_t ent = mem.rd<uintptr_t>(le + 0x78 * (i & 0x1FF));
            if (!ent || ent == local) continue;
            int hp = mem.rd<int>(ent + pr->hp);
            int tm = mem.rd<int>(ent + pr->team);
            if (hp <= 0 || hp > 100 || tm == myTeam) continue;
            if (pr->dormant && mem.rd<uint8_t>(ent + pr->dormant)) continue;
            float x = mem.rd<float>(ent + pr->origin), y = mem.rd<float>(ent + pr->origin + 4), z = mem.rd<float>(ent + pr->origin + 8);
            if (i % 8 == 0) Sleep(jit(2, 5)); // джиттер чтений: ломает фикс-интервальную эвристику RPM
            float hx, hy, fx, fy;
            if (!w2s(vm, x, y, z + 64, hx, hy)) continue;
            w2s(vm, x, y, z, fx, fy);
            int bh = (int)(fy - hy), bw = bh / 2;
            if (fEsp) {
                DI b = {}; b.t = 0; b.x = (int)hx - bw / 2; b.y = (int)hy; b.w = bw; b.h = bh; b.col = RGB(255, 60, 90); items.push_back(b);
                DI hbar = {}; hbar.t = 1; hbar.x = b.x - 4; hbar.y = b.y + bh - (int)(bh * hp / 100.f); hbar.w = b.x - 4; hbar.h = b.y + bh; hbar.col = RGB(0, 255, 136); items.push_back(hbar);
                DI t = {}; t.t = 2; t.x = b.x; t.y = b.y - 14; t.col = RGB(255, 255, 255); sprintf(t.txt, "%d hp", hp); items.push_back(t);
            }
            if (fRadar && local) {
                float dx = x - mem.rd<float>(local + pr->origin), dy = y - mem.rd<float>(local + pr->origin + 4);
                float ca = cosf(myAng), sa = sinf(myAng);
                float rx = dx * ca - dy * sa, ry = dx * sa + dy * ca;
                DI d = {}; d.t = 3; d.x = g_sw - 110 + (int)(rx / 40); d.y = 110 - (int)(ry / 40); d.col = RGB(255, 60, 90); items.push_back(d);
            }
            if (fAim || fTrig) {
                float ddx = hx - g_sw / 2, ddy = hy - g_sh / 2, d = sqrtf(ddx * ddx + ddy * ddy);
                if (d < bestD) { bestD = d; bestX = hx; bestY = hy; have = true; }
            }
            if (fTrig && pr->cross && local) {
                int cid = mem.rd<int>(local + pr->cross);
                if (cid == i + 1 && bestD < 40) { mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0); Sleep(20); mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0); }
            }
        }
        if (have && fAim && bestD < 260) moveMouse(bestX, bestY, 5.0f);
        EnterCriticalSection(&g_cs); g_items.swap(items); LeaveCriticalSection(&g_cs);
        if (g_ov) InvalidateRect(g_ov, 0, FALSE);
        Sleep(jit(3, 6));
    }
    return 0;
}
