// CheatForge per-game external — features compiled per binary via -DFEAT_*
#include <windows.h>
#include <tlhelp32.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include <random>
#ifdef GAME_CS2
#include "offsets_gen.h"
#endif

static std::mt19937 rng(std::random_device{}());
static int jit(int a, int b) { return a + (int)(rng() % (uint32_t)(b - a + 1)); }

// ---------- game config (selected at compile time) ----------
struct GCfg { const char* key; DWORD col; int tol; const short* recoil; int recoilN; };
#ifdef GAME_CS2
static const GCfg G = { "cs2", 0, 0, 0, 0 };
#elif defined(GAME_VALORANT)
static const short RC[] = { 0,-3, 0,-4, 1,-4, 0,-5, -1,-4, 0,-4, 1,-3, 0,-3 };
static const GCfg G = { "valorant", RGB(255,60,60), 70, RC, 8 };
#elif defined(GAME_RUST)
static const short RC[] = { 0,-4, 1,-4, 0,-5, -1,-4, 0,-4, 1,-3, 0,-4, 0,-3 };
static const GCfg G = { "rust", 0, 0, RC, 8 };
#elif defined(GAME_APEX)
static const GCfg G = { "apex", RGB(255,70,70), 75, 0, 0 };
#elif defined(GAME_PUBG)
static const short RC[] = { 0,-3, 0,-4, 1,-3, 0,-4, 0,-3, -1,-3, 0,-3, 0,-2 };
static const GCfg G = { "pubg", RGB(255,80,80), 70, RC, 8 };
#elif defined(GAME_FORTNITE)
static const short RC[] = { 0,-2, 1,-2, 0,-3, -1,-2, 0,-2, 1,-2, 0,-2, 0,-1 };
static const GCfg G = { "fortnite", 0, 0, RC, 8 };
#elif defined(GAME_GTA5)
static const short RC[] = { 0,-2, 1,-1, 0,-2, -1,-1, 0,-2, 1,-1, 0,-1, 0,-1 };
static const GCfg G = { "gta5", 0, 0, RC, 8 };
#elif defined(GAME_MINECRAFT)
static const GCfg G = { "minecraft", RGB(255,255,255), 45, 0, 0 };
#elif defined(GAME_TF2)
static const GCfg G = { "tf2", RGB(190,60,50), 60, 0, 0 };
#else
static const GCfg G = { "none", 0, 0, 0, 0 };
#endif

// ---------- memory (CS2 tier) ----------
#ifdef GAME_CS2
struct Mem {
    HANDLE hr = 0; DWORD pid = 0;
    bool attach(const char* proc) {
        HANDLE sn = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32 pe; pe.dwSize = sizeof(pe);
        if (Process32First(sn, &pe)) do { if (!_stricmp(pe.szExeFile, proc)) { pid = pe.th32ProcessID; break; } } while (Process32Next(sn, &pe));
        CloseHandle(sn); if (!pid) return false;
        hr = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
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
    template<class T> bool wr(uintptr_t a, T v) { HANDLE hw = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pid); if (!hw) return false; bool ok = WriteProcessMemory(hw, (LPVOID)a, &v, sizeof(T), 0) != 0; CloseHandle(hw); return ok; }
};
#endif

// ---------- overlay ----------
struct DI { int t; int x, y, w, h; DWORD col; char txt[24]; };
static std::vector<DI> g_items; static CRITICAL_SECTION g_cs; static HWND g_ov = 0; static int g_sw, g_sh;
static LRESULT CALLBACK OvProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_PAINT) {
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        RECT r; GetClientRect(h, &r);
        HBRUSH bg = CreateSolidBrush(RGB(255, 0, 255)); FillRect(dc, &r, bg); DeleteObject(bg);
        SetBkMode(dc, TRANSPARENT);
        EnterCriticalSection(&g_cs);
        for (auto& it : g_items) {
            if (it.t == 0) { HPEN p = CreatePen(PS_SOLID, 1, it.col); HGDIOBJ op = SelectObject(dc, p); HGDIOBJ ob = SelectObject(dc, GetStockObject(NULL_BRUSH)); Rectangle(dc, it.x, it.y, it.x + it.w, it.y + it.h); SelectObject(dc, op); SelectObject(dc, ob); DeleteObject(p); }
            else if (it.t == 2) { SetTextColor(dc, it.col); TextOutA(dc, it.x, it.y, it.txt, (int)strlen(it.txt)); }
            else { HBRUSH b = CreateSolidBrush(it.col); RECT rr = { it.x - 2, it.y - 2, it.x + 2, it.y + 2 }; FillRect(dc, &rr, b); DeleteObject(b); }
        }
        LeaveCriticalSection(&g_cs);
        EndPaint(h, &ps); return 0;
    }
    return DefWindowProcA(h, m, w, l);
}
static DWORD WINAPI OvThread(LPVOID) {
    WNDCLASSA wc = {}; wc.lpfnWndProc = OvProc; wc.hInstance = GetModuleHandleA(0); wc.lpszClassName = "EdgeUiWindow";
    RegisterClassA(&wc);
    g_ov = CreateWindowExA(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW, "EdgeUiWindow", "", WS_POPUP | WS_VISIBLE, 0, 0, g_sw, g_sh, 0, 0, wc.hInstance, 0);
    SetLayeredWindowAttributes(g_ov, RGB(255, 0, 255), 0, LWA_COLORKEY);
    MSG msg; while (GetMessageA(&msg, 0, 0, 0)) { TranslateMessage(&msg); DispatchMessageA(&msg); }
    return 0;
}
static void moveMouse(float tx, float ty, float smooth) {
    POINT c; GetCursorPos(&c);
    float nx = c.x + (tx - c.x) / smooth, ny = c.y + (ty - c.y) / smooth;
    INPUT in = {}; in.type = INPUT_MOUSE; in.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
    in.mi.dx = (LONG)((nx / GetSystemMetrics(SM_CXVIRTUALSCREEN)) * 65535);
    in.mi.dy = (LONG)((ny / GetSystemMetrics(SM_CYVIRTUALSCREEN)) * 65535);
    SendInput(1, &in, sizeof(in));
}

// ---------- screen capture + color clusters (no memory access at all) ----------
#if defined(FEAT_COLESP) || defined(FEAT_TRIGCOL) || defined(FEAT_AIMCOL)
static std::vector<uint32_t> g_cap; static int g_cw = 0, g_ch = 0;
static void capture() {
    HDC s = GetDC(0);
    g_cw = g_sw / 4; g_ch = g_sh / 4;
    std::vector<uint32_t> full((size_t)g_sw * g_sh);
    BITMAPINFO bi = {}; bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); bi.bmiHeader.biWidth = g_sw; bi.bmiHeader.biHeight = -g_sh; bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32;
    HDC m = CreateCompatibleDC(s); HBITMAP hb = CreateCompatibleBitmap(s, g_sw, g_sh);
    HGDIOBJ ob = SelectObject(m, hb); BitBlt(m, 0, 0, g_sw, g_sh, s, 0, 0, SRCCOPY);
    GetDIBits(m, hb, 0, g_sh, full.data(), &bi, DIB_RGB_COLORS);
    SelectObject(m, ob); DeleteObject(hb); DeleteDC(m); ReleaseDC(0, s);
    g_cap.assign((size_t)g_cw * g_ch, 0);
    for (int y = 0; y < g_ch; y++) for (int x = 0; x < g_cw; x++) g_cap[(size_t)y * g_cw + x] = full[(size_t)(y * 4) * g_sw + x * 4];
}
struct Cl { int x, y, n; };
static bool near_col(uint32_t c) {
    int r = c & 255, g = (c >> 8) & 255, b = (c >> 16) & 255;
    int tr = G.col & 255, tg = (G.col >> 8) & 255, tb = (G.col >> 16) & 255;
    return abs(r - tr) < G.tol && abs(g - tg) < G.tol && abs(b - tb) < G.tol;
}
static std::vector<Cl> clusters() {
    std::vector<Cl> out;
    for (int y = 0; y < g_ch; y++) for (int x = 0; x < g_cw; x++) {
        if (!near_col(g_cap[(size_t)y * g_cw + x])) continue;
        bool placed = false;
        for (auto& c : out) if (abs(c.x / c.n - x) < 8 && abs(c.y / c.n - y) < 10) { c.x += x; c.y += y; c.n++; placed = true; break; }
        if (!placed && out.size() < 12) out.push_back({ x, y, 1 });
    }
    return out;
}
#endif

static std::string readAll(const char* p) { FILE* f = fopen(p, "rb"); if (!f) return ""; fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET); std::string s(n, 0); if (n) fread(&s[0], 1, n, f); fclose(f); return s; }
static bool cfgHas(const std::string& s, const char* f) { std::string a = ":"; a += f; a += "\""; std::string b = "\""; b += f; b += "\""; return s.find(a) != std::string::npos || s.find(b) != std::string::npos; }
static bool gate(const std::string& cfg, const char* a, const char* b = 0) { if (cfg.empty()) return true; return cfgHas(cfg, a) || (b && cfgHas(cfg, b)); }

int main() {
    std::string cfg = readAll("config.json");
    printf("[cf] game=%s | compiled features:", G.key);
#ifdef FEAT_MEMESP
    printf(" memESP");
#endif
#ifdef FEAT_COLESP
    printf(" colorESP");
#endif
#ifdef FEAT_MEMAIM
    printf(" memAim");
#endif
#ifdef FEAT_AIMCOL
    printf(" colorAim");
#endif
#ifdef FEAT_TRIGMEM
    printf(" memTrig");
#endif
#ifdef FEAT_TRIGCOL
    printf(" colorTrig");
#endif
#ifdef FEAT_BHOPMEM
    printf(" memBhop");
#endif
#ifdef FEAT_BHOPIN
    printf(" inputBhop");
#endif
#ifdef FEAT_RCSMEM
    printf(" memRCS");
#endif
#ifdef FEAT_RECOIL
    printf(" recoilMacro");
#endif
#ifdef FEAT_RAPID
    printf(" rapidFire");
#endif
#ifdef FEAT_RADARMEM
    printf(" radar");
#endif
    printf("\n[cf] END = exit\n");
    g_sw = GetSystemMetrics(SM_CXSCREEN); g_sh = GetSystemMetrics(SM_CYSCREEN);
    InitializeCriticalSection(&g_cs);
    CreateThread(0, 0, OvThread, 0, 0, 0);
#ifdef GAME_CS2
    Mem mem; printf("[cf] waiting for cs2.exe ...\n");
    while (!mem.attach("cs2.exe")) Sleep(1000);
    uintptr_t base = mem.modBase("client.dll");
    printf("[cf] pid=%lu base=0x%llx\n", (unsigned long)mem.pid, (unsigned long long)base);
#endif
    float ppx = 0, ppy = 0; int pshots = 0, ridx = 0; bool lj = false; DWORD lastClick = 0;
    while (!(GetAsyncKeyState(VK_END) & 0x8000)) {
        std::vector<DI> items;
        bool lmb = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
        bool space = GetAsyncKeyState(VK_SPACE) & 0x8000;
#if defined(FEAT_COLESP) || defined(FEAT_TRIGCOL) || defined(FEAT_AIMCOL)
        capture();
        auto cls = clusters();
#ifdef FEAT_COLESP
        if (gate(cfg, "esp", "playeresp")) for (auto& c : cls) {
            if (c.n < 4) continue;
            DI b = {}; b.t = 0; b.x = c.x / c.n * 4 - 24; b.y = c.y / c.n * 4 - 40; b.w = 48; b.h = 80; b.col = RGB(255, 60, 90); items.push_back(b);
        }
#endif
#ifdef FEAT_TRIGCOL
        if (gate(cfg, "triggerbot") && !cls.empty()) {
            uint32_t center = g_cap[(size_t)(g_ch / 2) * g_cw + g_cw / 2];
            if (near_col(center) && GetTickCount() - lastClick > 120) { mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0); Sleep(18); mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0); lastClick = GetTickCount(); }
        }
#endif
#ifdef FEAT_AIMCOL
        if (gate(cfg, "aimbot", "aim_assist") && !cls.empty()) {
            int bi = 0; float bd = 1e9f;
            for (size_t i = 0; i < cls.size(); i++) { float dx = cls[i].x / cls[i].n * 4 - g_sw / 2, dy = cls[i].y / cls[i].n * 4 - g_sh / 2, d = sqrtf(dx * dx + dy * dy); if (d < bd) { bd = d; bi = (int)i; } }
            if (bd < 300) moveMouse(cls[bi].x / cls[bi].n * 4, cls[bi].y / cls[bi].n * 4, 6.0f);
        }
#endif
#endif
#ifdef FEAT_RECOIL
        if (gate(cfg, "rcs", "norecoil") && G.recoil && lmb) {
            INPUT in = {}; in.type = INPUT_MOUSE; in.mi.dwFlags = MOUSEEVENTF_MOVE;
            in.mi.dx = G.recoil[(ridx % G.recoilN) * 2]; in.mi.dy = G.recoil[(ridx % G.recoilN) * 2 + 1];
            SendInput(1, &in, sizeof(in)); ridx++;
        } else if (!lmb) ridx = 0;
#endif
#ifdef FEAT_RAPID
        if (gate(cfg, "rapid_fire") && lmb && GetTickCount() - lastClick > 60) { mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0); mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0); lastClick = GetTickCount(); }
#endif
#ifdef FEAT_BHOPIN
        if (gate(cfg, "bhop") && space) { keybd_event(VK_SPACE, 0, KEYEVENTF_KEYUP, 0); Sleep(jit(4, 9)); keybd_event(VK_SPACE, 0, 0, 0); }
#endif
#ifdef GAME_CS2
        float vm[16] = { 0 };
        for (int i = 0; i < 16; i++) vm[i] = mem.rd<float>(base + OFF_VIEWMAT + i * 4);
        uintptr_t local = mem.rd<uintptr_t>(base + OFF_PAWN);
        int myTeam = local ? mem.rd<int>(local + OFF_TEAM) : 0;
#ifdef FEAT_RCSMEM
        if (gate(cfg, "rcs", "norecoil") && local) {
            int sh = mem.rd<int>(local + OFF_SHOTS);
            float px = mem.rd<float>(local + OFF_PUNCH), py = mem.rd<float>(local + OFF_PUNCH + 4);
            if (sh > pshots) { INPUT in = {}; in.type = INPUT_MOUSE; in.mi.dwFlags = MOUSEEVENTF_MOVE; in.mi.dx = (LONG)-((px - ppx) * 2.f); in.mi.dy = (LONG)-((py - ppy) * 2.f); SendInput(1, &in, sizeof(in)); }
            pshots = sh; ppx = px; ppy = py;
        }
#endif
#ifdef FEAT_BHOPMEM
        if (gate(cfg, "bhop") && local) {
            int fl = mem.rd<int>(local + OFF_FLAGS);
            bool want = space && (fl & 1);
            if (want != lj) { mem.wr<int>(base + OFF_FORCEJUMP, want ? 6 : 0); lj = want; }
        }
#endif
#ifdef FEAT_NOFLASH
        if (gate(cfg, "remove_flash") && local) mem.wr<float>(local + OFF_FLASH, 0.f);
#endif
        float bd = 1e9f, bx = 0, by = 0; bool have = false;
        for (int i = 0; i < 64; i++) {
            uintptr_t le = mem.rd<uintptr_t>(base + OFF_ENTLIST + 0x8 * (i >> 9) + 0x10);
            if (!le) continue;
            uintptr_t ent = mem.rd<uintptr_t>(le + 0x78 * (i & 0x1FF));
            if (!ent || ent == local) continue;
            int hp = mem.rd<int>(ent + OFF_HP); int tm = mem.rd<int>(ent + OFF_TEAM);
            if (hp <= 0 || hp > 100 || tm == myTeam) continue;
            float x = mem.rd<float>(ent + OFF_ORIGIN), y = mem.rd<float>(ent + OFF_ORIGIN + 4), z = mem.rd<float>(ent + OFF_ORIGIN + 8);
            if (i % 8 == 0) Sleep(jit(2, 5));
            float w = vm[12] * x + vm[13] * y + vm[14] * z + vm[15];
            if (w < 0.01f) continue;
            float sx = vm[0] * x + vm[1] * y + vm[2] * z + vm[3], sy = vm[4] * x + vm[5] * y + vm[6] * z + vm[7];
            float hx = (g_sw / 2) * (1 + sx / w), hy = (g_sh / 2) * (1 - sy / w);
            float fx = hx, fy = (g_sh / 2) * (1 - (vm[4] * x + vm[5] * y + vm[6] * z + vm[7] - 64 * vm[6]) / w);
            int bh = (int)(fy - hy), bw = bh / 2;
#ifdef FEAT_MEMESP
            if (gate(cfg, "esp", "playeresp")) {
                DI b = {}; b.t = 0; b.x = (int)hx - bw / 2; b.y = (int)hy; b.w = bw; b.h = bh; b.col = RGB(255, 60, 90); items.push_back(b);
                DI t = {}; t.t = 2; t.x = b.x; t.y = b.y - 14; t.col = RGB(255, 255, 255); sprintf(t.txt, "%d hp", hp); items.push_back(t);
            }
#endif
#ifdef FEAT_RADARMEM
            if (gate(cfg, "radar") && local) {
                float myAng = mem.rd<float>(local + OFF_ANG + 4);
                float dx = x - mem.rd<float>(local + OFF_ORIGIN), dy = y - mem.rd<float>(local + OFF_ORIGIN + 4);
                float ca = cosf(myAng), sa = sinf(myAng);
                DI d = {}; d.t = 3; d.x = g_sw - 110 + (int)((dx * ca - dy * sa) / 40); d.y = 110 - (int)((dx * sa + dy * ca) / 40); d.col = RGB(255, 60, 90); items.push_back(d);
            }
#endif
#if defined(FEAT_MEMAIM) || defined(FEAT_TRIGMEM)
            float ddx = hx - g_sw / 2, ddy = hy - g_sh / 2, d = sqrtf(ddx * ddx + ddy * ddy);
            if (d < bd) { bd = d; bx = hx; by = hy; have = true; }
#ifdef FEAT_TRIGMEM
            if (gate(cfg, "triggerbot") && mem.rd<int>(local + OFF_CROSS) == i + 1 && d < 40 && GetTickCount() - lastClick > 120) { mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0); Sleep(18); mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0); lastClick = GetTickCount(); }
#endif
#endif
        }
#ifdef FEAT_MEMAIM
        if (gate(cfg, "aimbot") && have && bd < 260) moveMouse(bx, by, 5.0f);
#endif
#endif
        EnterCriticalSection(&g_cs); g_items.swap(items); LeaveCriticalSection(&g_cs);
        if (g_ov) InvalidateRect(g_ov, 0, FALSE);
        Sleep(jit(3, 6));
    }
    return 0;
}
