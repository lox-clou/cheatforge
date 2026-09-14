// CheatForge engine v8 — clean rewrite: win external (fresh-ntdll, ghost, etw, modal, lazy attach, hotkeys, HUD) + android ptrace-free scan/freeze
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include <random>
#include <cstdio>
#include <set>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#ifdef GAME_CS2
#include "offsets_gen.h"
#endif
#else
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/uio.h>
#include <android/log.h>
#define LOG(...) __android_log_print(ANDROID_LOG_INFO,"cf",__VA_ARGS__)
#endif
static std::mt19937 rng(std::random_device{}());
static int jit(int a,int b){return a+(int)(rng()%(uint32_t)(b-a+1));}
static std::string CFG; static bool cfgEmpty=true;
static void toggleTok(const char*id){if(g_selected.count(id))g_selected.erase(id);else g_selected.insert(id);saveCfg();}
static void toggleGroup(const char*a,const char*b){toggleTok(a);if(b)toggleTok(b);}
static bool on(const char*id){return cfgEmpty||CFG.find(std::string("\"")+id+"\"")!=std::string::npos;}
static std::string readAll(const char*p){FILE*f=fopen(p,"rb");if(!f)return"";fseek(f,0,SEEK_END);long n=ftell(f);fseek(f,0,SEEK_SET);std::string s(n,0);if(n)fread(&s[0],1,n,f);fclose(f);return s;}
#ifdef _WIN32
static void* g_nt=0;
static void mapFreshNt(){HANDLE hf=CreateFileA("C:\\Windows\\System32\\ntdll.dll",GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
 if(hf==INVALID_HANDLE_VALUE)return;HANDLE fm=CreateFileMappingA(hf,0,PAGE_READONLY,0,0,0);
 if(fm)g_nt=MapViewOfFile(fm,FILE_MAP_READ,0,0,0);CloseHandle(fm);CloseHandle(hf);}
static void* ntStub(const char*name){if(!g_nt)return 0;BYTE* b=(BYTE*)g_nt;DWORD pe=*(DWORD*)(b+0x3C);
 DWORD expRva=*(DWORD*)(b+pe+24+112);if(!expRva)return 0;
 WORD nsec=*(WORD*)(b+pe+6);WORD sizeOpt=*(WORD*)(b+pe+20);BYTE*sec=b+pe+24+sizeOpt;
 auto r2f=[=](DWORD rva)->BYTE*{for(int i=0;i<nsec;i++){DWORD va=*(DWORD*)(sec+i*40+8),vs=*(DWORD*)(sec+i*40+16),raw=*(DWORD*)(sec+i*40+20);
  if(rva>=va&&rva<va+vs)return (BYTE*)(b+raw+(rva-va));}return 0;};
 BYTE*exp=r2f(expRva);if(!exp)return 0;DWORD nnames=*(DWORD*)(exp+24);
 DWORD*names=(DWORD*)r2f(*(DWORD*)(exp+32));WORD*ord=(WORD*)r2f(*(DWORD*)(exp+36));DWORD*funcs=(DWORD*)r2f(*(DWORD*)(exp+28));
 if(!names||!ord||!funcs)return 0;
 for(DWORD i=0;i<nnames;i++){const char*nm=(const char*)r2f(names[i]);if(nm&&!strcmp(nm,name))return r2f(funcs[ord[i]]);}
 return 0;}
typedef LONG(NTAPI*RPMF)(HANDLE,PVOID,PVOID,ULONG,ULONG*);
typedef LONG(NTAPI*WPMF)(HANDLE,PVOID,PVOID,ULONG,ULONG*);
static RPMF fRead=0;static WPMF fWrite=0;static bool ntReady=false;
static void initNt(){mapFreshNt();fRead=(RPMF)ntStub("NtReadVirtualMemory");fWrite=(WPMF)ntStub("NtWriteVirtualMemory");ntReady=fRead&&fWrite;}
static const short PG[]={0,-3,0,-4,1,-4,0,-5,-1,-4,0,-4,1,-3,0,-3};
static const short PH[]={0,-2,0,-2,1,-2,0,-2,-1,-2,0,-2,1,-1,0,-1};
static const short PAK[]={0,-4,1,-4,0,-5,-1,-4,0,-4,1,-3,0,-4,0,-3};
static const short PM4[]={0,-3,0,-3,1,-3,0,-4,-1,-3,0,-3,1,-2,0,-2};
static const short PBT[]={0,-6,0,-7,0,-7,0,-6,0,-6,0,-5,0,-5,0,-4};
struct Mac{const char*id;int vk;int mode;int ms;};
static const Mac MACS[]={
#ifdef FEAT_I_CROUCH
{"i_crouch",VK_CONTROL,0,220},
#endif
#ifdef FEAT_I_LEAN
{"i_lean",0x51,0,240},
#endif
#ifdef FEAT_I_PRONE
{"i_prone",0x5A,0,300},
#endif
#ifdef FEAT_I_JUMP
{"i_jump",VK_SPACE,0,180},
#endif
#ifdef FEAT_I_WALK
{"i_walk",VK_SHIFT,1,0},
#endif
#ifdef FEAT_I_MELEE
{"i_melee",0x56,0,150},
#endif
#ifdef FEAT_I_AIMKEY
{"i_aimkey",VK_RBUTTON,0,160},
#endif
#ifdef FEAT_I_RELOAD
{"i_reload",0x52,0,260},
#endif
#ifdef FEAT_I_COVER
{"i_cover",0x43,0,320},
#endif
#ifdef FEAT_I_WEP
{"i_wep",0x58,0,140},
#endif
#ifdef FEAT_I_ENTER
{"i_enter",0x46,0,300},
#endif
#ifdef FEAT_I_TAP
{"i_tap",0,2,90},
#endif
#ifdef FEAT_I_STRAFE
{"i_strafe",0,2,260},
#endif
};
static const int NMAC=sizeof(MACS)/sizeof(MACS[0]);
struct GDef{const char*key;const char*proc;DWORD col,ore;int tol;};
#ifdef GAME_CS2
static const GDef G={"cs2","cs2.exe",0,0,0};
#elif defined(GAME_VALORANT)
static const GDef G={"valorant","VALORANT-Win64-Shipping.exe",RGB(255,60,60),0,70};
#elif defined(GAME_RUST)
static const GDef G={"rust","RustClient.exe",0,RGB(200,180,60),60};
#elif defined(GAME_APEX)
static const GDef G={"apex","r5apex.exe",RGB(255,70,70),0,75};
#elif defined(GAME_PUBG)
static const GDef G={"pubg","TslGame.exe",RGB(255,80,80),0,70};
#elif defined(GAME_FORTNITE)
static const GDef G={"fortnite","FortniteClient-Win64-Shipping.exe",0,0,0};
#elif defined(GAME_GTA5)
static const GDef G={"gta5","GTA5.exe",0,0,0};
#elif defined(GAME_MINECRAFT)
static const GDef G={"minecraft","javaw.exe",RGB(255,255,255),RGB(80,220,220),45};
#elif defined(GAME_TF2)
static const GDef G={"tf2","hl2.exe",RGB(190,60,50),0,60};
#else
static const GDef G={"none","none.exe",0,0,0};
#endif
#ifdef GAME_CS2
struct Mem{
 HANDLE hr=0;DWORD pid=0;
 bool attach(const char*p){HANDLE sn=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);PROCESSENTRY32 pe;pe.dwSize=sizeof(pe);
  if(Process32First(sn,&pe))do{if(!_stricmp(pe.szExeFile,p)){pid=pe.th32ProcessID;break;}}while(Process32Next(sn,&pe));
  CloseHandle(sn);if(!pid)return false;hr=OpenProcess(PROCESS_VM_READ|PROCESS_QUERY_INFORMATION,FALSE,pid);return hr!=0;}
 uintptr_t modBase(const char*m){HANDLE sn=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);
  if(sn==INVALID_HANDLE_VALUE)return 0;MODULEENTRY32 me;me.dwSize=sizeof(me);uintptr_t b=0;
  if(Module32First(sn,&me))do{if(!_stricmp(me.szModule,m)){b=(uintptr_t)me.modBaseAddr;break;}}while(Module32Next(sn,&me));
  CloseHandle(sn);return b;}
 template<class T>T rd(uintptr_t a){T v={};if(ntReady){ULONG o=0;fRead(hr,(PVOID)a,&v,sizeof(T),&o);}else ReadProcessMemory(hr,(LPCVOID)a,&v,sizeof(T),0);return v;}
 template<class T>bool wr(uintptr_t a,T v){HANDLE hw=OpenProcess(PROCESS_VM_WRITE|PROCESS_VM_OPERATION,FALSE,pid);if(!hw)return false;
  bool ok;if(ntReady){ULONG o=0;ok=fWrite(hw,(PVOID)a,&v,sizeof(T),&o)==0;}else ok=WriteProcessMemory(hw,(LPVOID)a,&v,sizeof(T),0)!=0;
  CloseHandle(hw);return ok;}
};
#endif
struct DI{int t;int x,y,w,h;DWORD col;char txt[48];};
static std::vector<DI> g_items;static CRITICAL_SECTION g_cs;static HWND g_ov=0;static int g_sw,g_sh;

// ===== CheatForge GUI menu =====
static bool g_menuOpen=true;
static int g_activeTab=0;
struct Feat{const char*id;const char*name;int tab;};
static std::vector<Feat> g_feats;
static const char* TABN[]={"AIM","VISUAL","MOVE","MISC"};
#ifdef GAME_CS2
static void initFeats(){g_feats={
 {"m_aim","Aimbot",0},{"m_head","Head bias",0},{"m_smooth","Smooth",0},{"m_fov","FOV limit",0},{"m_trig","Triggerbot",0},{"m_rcs","Recoil ctrl",0},
 {"m_esp","ESP boxes",1},{"m_hp","Health bars",1},{"m_dist","Distance",1},{"m_radar","World radar",1},{"d_fovc","FOV circle",1},{"d_cross","Crosshair",1},{"d_trac","Tracers",1},
 {"m_bhop","Bunny hop",2},{"m_strafe","Auto strafe",2},
 {"m_noflash","No flash",3},{"i_rapid","Rapid fire",3},{"d_water","Watermark",3},{"d_fps","FPS meter",3},{"d_timer","Timer",3}};}
#elif defined(GAME_VALORANT)
static void initFeats(){g_feats={
 {"c_aim","Aimbot",0},{"c_head","Head bias",0},{"c_smooth","Smooth",0},{"c_trig","Triggerbot",0},{"i_rec","Recoil",0},
 {"c_esp","ESP",1},{"c_radar","Radar",1},{"d_fovc","FOV circle",1},{"d_cross","Crosshair",1},
 {"i_rapid","Rapid fire",3},{"d_water","Watermark",3},{"d_fps","FPS meter",3}};}
#elif defined(GAME_RUST)
static void initFeats(){g_feats={
 {"i_rec","Recoil AK",0},{"i_rak","AK pattern",0},{"i_rm4","M4 pattern",0},{"i_rapid","Rapid fire",0},
 {"c_ore","Ore ESP",1},{"d_cross","Crosshair",1},{"d_water","Watermark",1},
 {"i_bhop","Bhop",2},{"i_strafe","Strafe",2},
 {"d_fps","FPS meter",3}};}
#elif defined(GAME_APEX)
static void initFeats(){g_feats={
 {"c_aim","Aimbot",0},{"c_head","Head bias",0},{"c_trig","Triggerbot",0},{"i_rec","Recoil",0},
 {"c_esp","ESP glow",1},{"c_radar","Radar",1},{"d_cross","Crosshair",1},
 {"i_bhop","Bhop",2},
 {"d_water","Watermark",3},{"d_fps","FPS meter",3}};}
#elif defined(GAME_PUBG)
static void initFeats(){g_feats={
 {"c_aim","Aimbot",0},{"i_rec","Recoil M416",0},{"i_rak","AKM pattern",0},{"c_trig","Triggerbot",0},
 {"c_esp","ESP",1},{"c_radar","Radar",1},{"d_cross","Crosshair",1},
 {"i_rapid","Rapid fire",3},{"d_water","Watermark",3}};}
#elif defined(GAME_FORTNITE)
static void initFeats(){g_feats={
 {"i_rec","Recoil AR",0},{"i_rak","AR pattern",0},{"i_rm4","SMG pattern",0},{"i_rapid","Rapid fire",0},
 {"d_cross","Crosshair",1},{"d_water","Watermark",1},
 {"i_bhop","Bhop",2},{"i_strafe","Strafe",2},
 {"d_fps","FPS meter",3}};}
#elif defined(GAME_GTA5)
static void initFeats(){g_feats={
 {"i_rec","Recoil",0},{"i_rapid","Rapid fire",0},{"i_aimkey","Aim spam",0},
 {"d_cross","Crosshair",1},{"d_water","Watermark",1},
 {"i_bhop","Jump spam",2},
 {"d_fps","FPS meter",3}};}
#elif defined(GAME_MINECRAFT)
static void initFeats(){g_feats={
 {"c_aim","Aimbot",0},{"c_trig","Triggerbot",0},{"i_rapid","CPS boost",0},
 {"c_esp","Nametag ESP",1},{"c_ore","Ore ESP",1},{"c_radar","Radar",1},
 {"i_bhop","Bhop",2},
 {"d_water","Watermark",3},{"d_fps","FPS meter",3}};}
#elif defined(GAME_TF2)
static void initFeats(){g_feats={
 {"c_aim","Aimbot",0},{"i_rec","Recoil",0},{"c_trig","Triggerbot",0},
 {"c_esp","ESP red team",1},{"c_radar","Radar",1},{"d_cross","Crosshair",1},
 {"i_bhop","Bhop",2},
 {"d_water","Watermark",3}};}
#else
static void initFeats(){}
#endif
static void setMenuStyle(bool open){LONG ex=WS_EX_LAYERED|WS_EX_TOPMOST|WS_EX_TOOLWINDOW;if(!open)ex|=WS_EX_TRANSPARENT;SetWindowLongPtrA(g_ov,GWL_EXSTYLE,ex);}
static void menuRects(int&mx,int&my,int&mw,int&mh){mx=60;my=60;mw=440;mh=560;}
static void drawMenu(HDC dc){
 if(!g_menuOpen)return;
 int mx,my,mw,mh;menuRects(mx,my,mw,mh);
 HBRUSH pb=CreateSolidBrush(RGB(16,18,26));RECT pr={mx,my,mx+mw,my+mh};FillRect(dc,&pr,pb);DeleteObject(pb);
 HPEN bo=CreatePen(PS_SOLID,2,RGB(0,240,255));HGDIOBJ op=SelectObject(dc,bo);HGDIOBJ ob=SelectObject(dc,GetStockObject(NULL_BRUSH));Rectangle(dc,mx,my,mx+mw,my+mh);SelectObject(dc,op);SelectObject(dc,ob);DeleteObject(bo);
 SetBkMode(dc,TRANSPARENT);
 SetTextColor(dc,RGB(0,240,255));TextOutA(dc,mx+16,my+10,"CHEATFORGE",10);
 SetTextColor(dc,RGB(110,116,140));TextOutA(dc,mx+150,my+12,"INSERT close | END exit",22);
 int tw=mw/4;
 for(int t=0;t<4;t++){RECT tr={mx+t*tw,my+34,mx+(t+1)*tw,my+62};
  HBRUSH tb=CreateSolidBrush(t==g_activeTab?RGB(0,240,255):RGB(28,32,44));FillRect(dc,&tr,tb);DeleteObject(tb);
  SetTextColor(dc,t==g_activeTab?RGB(0,0,0):RGB(160,166,190));TextOutA(dc,tr.left+16,tr.top+8,TABN[t],(int)strlen(TABN[t]));}
 int y=my+74;
 for(auto&f:g_feats){if(f.tab!=g_activeTab)continue;
  bool isOn=on(f.id);
  RECT cb={mx+16,y+6,mx+32,y+22};
  HBRUSH cb_b=CreateSolidBrush(isOn?RGB(0,255,136):RGB(40,44,58));FillRect(dc,&cb,cb_b);DeleteObject(cb_b);
  HPEN cbp=CreatePen(PS_SOLID,1,RGB(90,96,120));HGDIOBJ cp=SelectObject(dc,cbp);HGDIOBJ cb2=SelectObject(dc,GetStockObject(NULL_BRUSH));Rectangle(dc,cb.left,cb.top,cb.right,cb.bottom);SelectObject(dc,cp);SelectObject(dc,cb2);DeleteObject(cbp);
  SetTextColor(dc,isOn?RGB(230,235,245):RGB(120,126,150));TextOutA(dc,mx+42,y+8,f.name,(int)strlen(f.name));
  y+=30;
  if(y>my+mh-20)break;}
}
static void menuClick(int x,int y){
 int mx,my,mw,mh;menuRects(mx,my,mw,mh);
 if(x<mx||x>mx+mw||y<my||y>my+mh)return;
 if(y>=my+34&&y<my+62){int tw=mw/4;int t=(x-mx)/tw;if(t>=0&&t<4)g_activeTab=t;return;}
 int yy=my+74;
 for(auto&f:g_feats){if(f.tab!=g_activeTab)continue;
  if(y>=yy&&y<yy+30){toggleTok(f.id);return;}
  yy+=30;}
}


// ===== CheatForge GUI menu (separate real topmost window) =====
static bool g_menuOpen=true;
static int g_activeTab=0;
static HWND g_menu=0;
static const int MENU_X=60,MENU_Y=60,MENU_W=440,MENU_H=560;
struct Feat{const char*id;const char*name;int tab;};
static std::vector<Feat> g_feats;
static const char* TABN[]={"AIM","VISUAL","MOVE","MISC"};
#ifdef GAME_CS2
static void initFeats(){g_feats={
 {"m_aim","Aimbot",0},{"m_head","Head bias",0},{"m_smooth","Smooth",0},{"m_fov","FOV limit",0},{"m_trig","Triggerbot",0},{"m_rcs","Recoil ctrl",0},
 {"m_esp","ESP boxes",1},{"m_hp","Health bars",1},{"m_dist","Distance",1},{"m_radar","World radar",1},{"d_fovc","FOV circle",1},{"d_cross","Crosshair",1},{"d_trac","Tracers",1},
 {"m_bhop","Bunny hop",2},{"m_strafe","Auto strafe",2},
 {"m_noflash","No flash",3},{"i_rapid","Rapid fire",3},{"d_water","Watermark",3},{"d_fps","FPS meter",3},{"d_timer","Timer",3}};}
#elif defined(GAME_VALORANT)
static void initFeats(){g_feats={
 {"c_aim","Aimbot",0},{"c_head","Head bias",0},{"c_smooth","Smooth",0},{"c_trig","Triggerbot",0},{"i_rec","Recoil",0},
 {"c_esp","ESP",1},{"c_radar","Radar",1},{"d_fovc","FOV circle",1},{"d_cross","Crosshair",1},
 {"i_rapid","Rapid fire",3},{"d_water","Watermark",3},{"d_fps","FPS meter",3}};}
#elif defined(GAME_RUST)
static void initFeats(){g_feats={
 {"i_rec","Recoil AK",0},{"i_rak","AK pattern",0},{"i_rm4","M4 pattern",0},{"i_rapid","Rapid fire",0},
 {"c_ore","Ore ESP",1},{"d_cross","Crosshair",1},{"d_water","Watermark",1},
 {"i_bhop","Bhop",2},{"i_strafe","Strafe",2},
 {"d_fps","FPS meter",3}};}
#elif defined(GAME_APEX)
static void initFeats(){g_feats={
 {"c_aim","Aimbot",0},{"c_head","Head bias",0},{"c_trig","Triggerbot",0},{"i_rec","Recoil",0},
 {"c_esp","ESP glow",1},{"c_radar","Radar",1},{"d_cross","Crosshair",1},
 {"i_bhop","Bhop",2},
 {"d_water","Watermark",3},{"d_fps","FPS meter",3}};}
#elif defined(GAME_PUBG)
static void initFeats(){g_feats={
 {"c_aim","Aimbot",0},{"i_rec","Recoil M416",0},{"i_rak","AKM pattern",0},{"c_trig","Triggerbot",0},
 {"c_esp","ESP",1},{"c_radar","Radar",1},{"d_cross","Crosshair",1},
 {"i_rapid","Rapid fire",3},{"d_water","Watermark",3}};}
#elif defined(GAME_FORTNITE)
static void initFeats(){g_feats={
 {"i_rec","Recoil AR",0},{"i_rak","AR pattern",0},{"i_rm4","SMG pattern",0},{"i_rapid","Rapid fire",0},
 {"d_cross","Crosshair",1},{"d_water","Watermark",1},
 {"i_bhop","Bhop",2},{"i_strafe","Strafe",2},
 {"d_fps","FPS meter",3}};}
#elif defined(GAME_GTA5)
static void initFeats(){g_feats={
 {"i_rec","Recoil",0},{"i_rapid","Rapid fire",0},{"i_aimkey","Aim spam",0},
 {"d_cross","Crosshair",1},{"d_water","Watermark",1},
 {"i_bhop","Jump spam",2},
 {"d_fps","FPS meter",3}};}
#elif defined(GAME_MINECRAFT)
static void initFeats(){g_feats={
 {"c_aim","Aimbot",0},{"c_trig","Triggerbot",0},{"i_rapid","CPS boost",0},
 {"c_esp","Nametag ESP",1},{"c_ore","Ore ESP",1},{"c_radar","Radar",1},
 {"i_bhop","Bhop",2},
 {"d_water","Watermark",3},{"d_fps","FPS meter",3}};}
#elif defined(GAME_TF2)
static void initFeats(){g_feats={
 {"c_aim","Aimbot",0},{"i_rec","Recoil",0},{"c_trig","Triggerbot",0},
 {"c_esp","ESP red team",1},{"c_radar","Radar",1},{"d_cross","Crosshair",1},
 {"i_bhop","Bhop",2},
 {"d_water","Watermark",3}};}
#else
static void initFeats(){}
#endif
static void drawMenuTo(HDC dc){
 HBRUSH pb=CreateSolidBrush(RGB(16,18,26));RECT pr={0,0,MENU_W,MENU_H};FillRect(dc,&pr,pb);DeleteObject(pb);
 HPEN bo=CreatePen(PS_SOLID,2,RGB(0,240,255));HGDIOBJ op=SelectObject(dc,bo);HGDIOBJ ob=SelectObject(dc,GetStockObject(NULL_BRUSH));Rectangle(dc,0,0,MENU_W,MENU_H);SelectObject(dc,op);SelectObject(dc,ob);DeleteObject(bo);
 SetBkMode(dc,TRANSPARENT);
 SetTextColor(dc,RGB(0,240,255));TextOutA(dc,16,10,"CHEATFORGE",10);
 SetTextColor(dc,RGB(110,116,140));TextOutA(dc,150,12,"INSERT close | END exit",22);
 int tw=MENU_W/4;
 for(int t=0;t<4;t++){RECT tr={t*tw,34,(t+1)*tw,62};
  HBRUSH tb=CreateSolidBrush(t==g_activeTab?RGB(0,240,255):RGB(28,32,44));FillRect(dc,&tr,tb);DeleteObject(tb);
  SetTextColor(dc,t==g_activeTab?RGB(0,0,0):RGB(160,166,190));TextOutA(dc,tr.left+16,tr.top+8,TABN[t],(int)strlen(TABN[t]));}
 int y=74;
 for(auto&f:g_feats){if(f.tab!=g_activeTab)continue;
  bool isOn=on(f.id);
  RECT cb={16,y+6,32,y+22};
  HBRUSH cb_b=CreateSolidBrush(isOn?RGB(0,255,136):RGB(40,44,58));FillRect(dc,&cb,cb_b);DeleteObject(cb_b);
  HPEN cbp=CreatePen(PS_SOLID,1,RGB(90,96,120));HGDIOBJ cp=SelectObject(dc,cbp);HGDIOBJ cb2=SelectObject(dc,GetStockObject(NULL_BRUSH));Rectangle(dc,cb.left,cb.top,cb.right,cb.bottom);SelectObject(dc,cp);SelectObject(dc,cb2);DeleteObject(cbp);
  SetTextColor(dc,isOn?RGB(230,235,245):RGB(120,126,150));TextOutA(dc,42,y+8,f.name,(int)strlen(f.name));
  y+=30;
  if(y>MENU_H-20)break;}
}
static void menuClick(int x,int y){
 if(y>=34&&y<62){int tw=MENU_W/4;int t=x/tw;if(t>=0&&t<4){g_activeTab=t;InvalidateRect(g_menu,0,FALSE);}return;}
 int yy=74;
 for(auto&f:g_feats){if(f.tab!=g_activeTab)continue;
  if(y>=yy&&y<yy+30){toggleTok(f.id);InvalidateRect(g_menu,0,FALSE);return;}
  yy+=30;}
}
static LRESULT CALLBACK MenuProc(HWND h,UINT m,WPARAM w,LPARAM l){
 if(m==WM_PAINT){PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);drawMenuTo(dc);EndPaint(h,&ps);return 0;}
 if(m==WM_ERASEBKGND)return 1;
 if(m==WM_LBUTTONDOWN){menuClick((short)LOWORD(l),(short)HIWORD(l));return 0;}
 return DefWindowProcA(h,m,w,l);
}
static void setMenuVisible(bool v){if(!g_menu)return;ShowWindow(g_menu,v?SW_SHOW:SW_HIDE);if(v)SetForegroundWindow(g_menu);}
static void createMenuWindow(){
 WNDCLASSA wc={};wc.lpfnWndProc=MenuProc;wc.hInstance=GetModuleHandleA(0);wc.lpszClassName="CheatForgeMenu";wc.hbrBackground=0;
 RegisterClassA(&wc);
 g_menu=CreateWindowExA(WS_EX_TOPMOST|WS_EX_TOOLWINDOW,"CheatForgeMenu","",WS_POPUP,MENU_X,MENU_Y,MENU_W,MENU_H,0,0,wc.hInstance,0);
}


// ===== CheatForge GUI v2 — draggable topmost, modern design =====
static bool g_menuOpen=true;
static int g_activeTab=0;
static int g_hoverIdx=-1;
static HWND g_menu=0;
static const int MENU_W=500, MENU_H=640, MENU_X=80, MENU_Y=80;
struct Feat{const char*id;const char*name;int tab;};
static std::vector<Feat> g_feats;
static const char* TABN[]={"AIMBOT","VISUALS","MOVEMENT","MISC"};
static void initFeats(){
#ifdef GAME_CS2
 g_feats={{"m_aim","Silent Aim",0},{"m_head","Head Priority",0},{"m_smooth","Human Smooth",0},
  {"m_fov","FOV Limit",0},{"m_trig","Triggerbot",0},{"m_rcs","Recoil Ctrl",0},
  {"m_esp","Player ESP",1},{"m_hp","Health Bars",1},{"m_dist","Distance",1},
  {"m_radar","World Radar",1},{"d_fovc","FOV Circle",1},{"d_cross","Crosshair",1},
  {"d_trac","Tracers",1},{"d_hitm","Hitmarker",1},
  {"m_bhop","Bunny Hop",2},{"m_strafe","Auto Strafe",2},
  {"m_noflash","No Flash",3},{"i_rapid","Rapid Fire",3},{"d_water","Watermark",3},
  {"d_fps","FPS Meter",3},{"d_timer","Session Timer",3}};
#elif defined(GAME_VALORANT)
 g_feats={{"c_aim","Aimbot",0},{"c_head","Head Priority",0},{"c_smooth","Smooth",0},
  {"c_trig","Triggerbot",0},{"i_rec","Recoil Macro",0},
  {"c_esp","ESP Outlines",1},{"c_radar","Radar",1},{"d_fovc","FOV Circle",1},
  {"d_cross","Crosshair",1},{"d_hitm","Hitmarker",1},
  {"i_rapid","Rapid Fire",3},{"d_water","Watermark",3},{"d_fps","FPS Meter",3}};
#elif defined(GAME_RUST)
 g_feats={{"i_rec","Recoil AK",0},{"i_rak","AK Pattern",0},{"i_rm4","M4 Pattern",0},
  {"i_rapid","Rapid Fire",0},
  {"c_ore","Ore ESP",1},{"d_cross","Crosshair",1},{"d_water","Watermark",1},
  {"i_bhop","Bunny Hop",2},{"i_strafe","Air Strafe",2},{"i_tap","Tap Strafe",2},
  {"d_fps","FPS Meter",3}};
#elif defined(GAME_APEX)
 g_feats={{"c_aim","Aimbot",0},{"c_head","Head Priority",0},{"c_trig","Triggerbot",0},
  {"i_rec","Recoil Macro",0},{"i_tap","Tap Strafe",0},
  {"c_esp","ESP Glow",1},{"c_radar","Radar",1},{"d_cross","Crosshair",1},
  {"i_bhop","Bhop",2},{"i_strafe","Strafe",2},
  {"d_water","Watermark",3},{"d_fps","FPS Meter",3}};
#elif defined(GAME_PUBG)
 g_feats={{"c_aim","Aimbot",0},{"i_rec","M416 Recoil",0},{"i_rak","AKM Pattern",0},
  {"c_trig","Triggerbot",0},
  {"c_esp","ESP",1},{"c_radar","Radar",1},{"d_cross","Crosshair",1},
  {"i_rapid","Rapid Fire",3},{"d_water","Watermark",3}};
#elif defined(GAME_FORTNITE)
 g_feats={{"i_rec","AR Recoil",0},{"i_rak","AR Pattern",0},{"i_rm4","SMG Pattern",0},
  {"i_rapid","Rapid Fire",0},
  {"d_cross","Crosshair",1},{"d_water","Watermark",1},
  {"i_bhop","Bhop",2},{"i_strafe","Strafe",2},
  {"d_fps","FPS Meter",3}};
#elif defined(GAME_GTA5)
 g_feats={{"i_rec","Recoil",0},{"i_rapid","Rapid Fire",0},{"i_aimkey","Aim Spam",0},
  {"d_cross","Crosshair",1},{"d_water","Watermark",1},
  {"i_bhop","Jump Spam",2},
  {"d_fps","FPS Meter",3}};
#elif defined(GAME_MINECRAFT)
 g_feats={{"c_aim","Aimbot",0},{"c_trig","Triggerbot",0},{"i_rapid","CPS Boost",0},
  {"c_esp","Nametag ESP",1},{"c_ore","Ore ESP",1},{"c_radar","Radar",1},
  {"i_bhop","Bhop",2},
  {"d_water","Watermark",3},{"d_fps","FPS Meter",3}};
#elif defined(GAME_TF2)
 g_feats={{"c_aim","Aimbot",0},{"i_rec","Recoil",0},{"c_trig","Triggerbot",0},
  {"c_esp","Red Team ESP",1},{"c_radar","Radar",1},{"d_cross","Crosshair",1},
  {"i_bhop","Bhop",2},
  {"d_water","Watermark",3}};
#endif
 std::string f=readAll("config.json");
 for(auto&ft:g_feats){std::string q=std::string("\"")+ft.id+"\"";
  if(f.find(q)!=std::string::npos)g_selected.insert(ft.id);}
}
static void drawMenuTo(HDC dc){
 for(int y=0;y<MENU_H;y++){int v=15+(y*6/MENU_H);
  HBRUSH b=CreateSolidBrush(RGB(v,v+2,v+8));RECT r={0,y,MENU_W,y+1};FillRect(dc,&r,b);DeleteObject(b);}
 HPEN bo=CreatePen(PS_SOLID,2,RGB(0,240,255));HGDIOBJ op=SelectObject(dc,bo);
 HGDIOBJ ob=SelectObject(dc,GetStockObject(NULL_BRUSH));
 Rectangle(dc,1,1,MENU_W-1,MENU_H-1);SelectObject(dc,op);SelectObject(dc,ob);DeleteObject(bo);
 SetBkMode(dc,TRANSPARENT);
 SetTextColor(dc,RGB(0,60,80));TextOutA(dc,19,15,"CHEATFORGE",10);
 SetTextColor(dc,RGB(0,240,255));TextOutA(dc,18,14,"CHEATFORGE",10);
 SetTextColor(dc,RGB(110,116,140));TextOutA(dc,160,16,"external  |  INSERT to hide",27);
 {RECT cr={MENU_W-34,8,MENU_W-10,32};HBRUSH cb=CreateSolidBrush(RGB(255,60,90));FillRect(dc,&cr,cb);DeleteObject(cb);
  SetTextColor(dc,RGB(20,0,0));TextOutA(dc,MENU_W-26,13,"X",1);}
 int tabW=MENU_W/4-4;
 for(int t=0;t<4;t++){int x=4+t*(tabW+4);RECT tr={x,44,x+tabW,70};bool active=t==g_activeTab;
  HBRUSH tb=CreateSolidBrush(active?RGB(0,240,255):RGB(26,29,40));FillRect(dc,&tr,tb);DeleteObject(tb);
  if(!active){HPEN tp=CreatePen(PS_SOLID,1,RGB(50,54,70));HGDIOBJ op2=SelectObject(dc,tp);
   HGDIOBJ ob2=SelectObject(dc,GetStockObject(NULL_BRUSH));Rectangle(dc,tr.left,tr.top,tr.right,tr.bottom);
   SelectObject(dc,op2);SelectObject(dc,ob2);DeleteObject(tp);}
  SetTextColor(dc,active?RGB(0,0,0):RGB(160,166,190));TextOutA(dc,tr.left+12,tr.top+7,TABN[t],(int)strlen(TABN[t]));}
 HPEN dv=CreatePen(PS_SOLID,1,RGB(35,38,52));HGDIOBJ dvop=SelectObject(dc,dv);
 MoveToEx(dc,10,82,0);LineTo(dc,MENU_W-10,82);SelectObject(dc,dvop);DeleteObject(dv);
 int y=94,idx=0;
 for(auto&f:g_feats){if(f.tab!=g_activeTab){idx++;continue;}
  bool isOn=on(f.id);bool hov=idx==g_hoverIdx;
  RECT rr={8,y,MENU_W-8,y+34};
  if(hov){HBRUSH hb=CreateSolidBrush(RGB(28,32,46));FillRect(dc,&rr,hb);DeleteObject(hb);}
  SetTextColor(dc,isOn?RGB(235,240,250):RGB(140,146,170));TextOutA(dc,22,y+10,f.name,(int)strlen(f.name));
  int swx=MENU_W-60,swy=y+8,sww=36,swh=18;
  HBRUSH sbg=CreateSolidBrush(isOn?RGB(0,240,255):RGB(50,54,70));
  HGDIOBJ sbp=SelectObject(dc,sbg);HGDIOBJ sbpen=SelectObject(dc,GetStockObject(NULL_PEN));
  RoundRect(dc,swx,swy,swx+sww,swy+swh,18,18);
  SelectObject(dc,sbp);SelectObject(dc,sbpen);DeleteObject(sbg);
  int bx=isOn?swx+sww-16:swx+2;
  HBRUSH ball=CreateSolidBrush(RGB(255,255,255));HGDIOBJ bbp=SelectObject(dc,ball);
  HGDIOBJ bpen=SelectObject(dc,GetStockObject(NULL_PEN));
  Ellipse(dc,bx,swy+2,bx+14,swy+16);
  SelectObject(dc,bbp);SelectObject(dc,bpen);DeleteObject(ball);
  y+=36;idx++;if(y>MENU_H-30)break;}
 SetTextColor(dc,RGB(80,86,110));TextOutA(dc,12,MENU_H-22,"CheatForge v9  |  END = exit cheat",34);
}
static int featAt(int x,int y){if(y<94||x<8||x>MENU_W-8)return -1;int idx=0;int yy=94;
 for(auto&f:g_feats){if(f.tab!=g_activeTab){idx++;continue;}
  if(y>=yy&&y<yy+34)return idx;yy+=36;idx++;}return -1;}
static void menuClick(int x,int y){
 if(x>=MENU_W-34&&y>=8&&y<32){setMenuVisible(false);g_menuOpen=false;return;}
 if(y>=44&&y<70){int tabW=MENU_W/4-4;int t=(x-4)/(tabW+4);if(t>=0&&t<4){g_activeTab=t;g_hoverIdx=-1;InvalidateRect(g_menu,0,FALSE);}return;}
 int idx=featAt(x,y);if(idx>=0){int j=0;for(auto&f:g_feats){if(f.tab==g_activeTab){if(j==idx){toggleTok(f.id);InvalidateRect(g_menu,0,FALSE);return;}}j++;}}}
static LRESULT CALLBACK MenuProc(HWND h,UINT m,WPARAM w,LPARAM l){
 if(m==WM_NCHITTEST){POINT p={(short)LOWORD(l),(short)HIWORD(l)};ScreenToClient(h,&p);
  if(p.x>=MENU_W-34&&p.y<34)return HTCLIENT;if(p.y<40)return HTCAPTION;return HTCLIENT;}
 if(m==WM_PAINT){PAINTSTRUCT ps;HDC wdc=BeginPaint(h,&ps);HDC dc=CreateCompatibleDC(wdc);
  HBITMAP bm=CreateCompatibleBitmap(wdc,MENU_W,MENU_H);HGDIOBJ ob=SelectObject(dc,bm);
  drawMenuTo(dc);BitBlt(wdc,0,0,MENU_W,MENU_H,dc,0,0,SRCCOPY);
  SelectObject(dc,ob);DeleteObject(bm);DeleteDC(dc);EndPaint(h,&ps);return 0;}
 if(m==WM_ERASEBKGND)return 1;
 if(m==WM_LBUTTONDOWN){menuClick((short)LOWORD(l),(short)HIWORD(l));return 0;}
 if(m==WM_MOUSEMOVE){int ni=featAt((short)LOWORD(l),(short)HIWORD(l));
  if(ni!=g_hoverIdx){g_hoverIdx=ni;InvalidateRect(h,0,FALSE);}
  TRACKMOUSEEVENT tme={sizeof(tme),TME_LEAVE,h,0};TrackMouseEvent(&tme);return 0;}
 if(m==WM_MOUSELEAVE){g_hoverIdx=-1;InvalidateRect(h,0,FALSE);return 0;}
 return DefWindowProcA(h,m,w,l);}
static void setMenuVisible(bool v){if(!g_menu)return;ShowWindow(g_menu,v?SW_SHOW:SW_HIDE);}
static void createMenuWindow(){WNDCLASSA wc={};wc.lpfnWndProc=MenuProc;wc.hInstance=GetModuleHandleA(0);
 wc.lpszClassName="CheatForgeMenu";wc.hbrBackground=0;RegisterClassA(&wc);
 g_menu=CreateWindowExA(WS_EX_TOPMOST|WS_EX_TOOLWINDOW,"CheatForgeMenu","",WS_POPUP,
  MENU_X,MENU_Y,MENU_W,MENU_H,0,0,wc.hInstance,0);}

static LRESULT CALLBACK OvProc(HWND h,UINT m,WPARAM w,LPARAM l){
 if(m==WM_LBUTTONDOWN&&g_menuOpen){menuClick((short)LOWORD(l),(short)HIWORD(l));InvalidateRect(h,0,FALSE);return 0;}
 if(m==WM_PAINT){PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);RECT r;GetClientRect(h,&r);
  HBRUSH bg=CreateSolidBrush(RGB(255,0,255));FillRect(dc,&r,bg);DeleteObject(bg);SetBkMode(dc,TRANSPARENT);
  EnterCriticalSection(&g_cs);
  for(auto&it:g_items){
   if(it.t==0){HPEN p=CreatePen(PS_SOLID,1,it.col);HGDIOBJ op=SelectObject(dc,p);HGDIOBJ ob=SelectObject(dc,GetStockObject(NULL_BRUSH));Rectangle(dc,it.x,it.y,it.x+it.w,it.y+it.h);SelectObject(dc,op);SelectObject(dc,ob);DeleteObject(p);}
   else if(it.t==1){HPEN p=CreatePen(PS_SOLID,1,it.col);HGDIOBJ op=SelectObject(dc,p);MoveToEx(dc,it.x,it.y,0);LineTo(dc,it.w,it.h);SelectObject(dc,op);DeleteObject(p);}
   else if(it.t==2){SetTextColor(dc,it.col);TextOutA(dc,it.x,it.y,it.txt,(int)strlen(it.txt));}
   else if(it.t==3){HBRUSH b=CreateSolidBrush(it.col);RECT rr={it.x-2,it.y-2,it.x+2,it.y+2};FillRect(dc,&rr,b);DeleteObject(b);}
   else{HPEN p=CreatePen(PS_SOLID,1,it.col);HGDIOBJ op=SelectObject(dc,p);HGDIOBJ ob=SelectObject(dc,GetStockObject(NULL_BRUSH));Ellipse(dc,it.x-it.w,it.y-it.w,it.x+it.w,it.y+it.w);SelectObject(dc,op);SelectObject(dc,ob);DeleteObject(p);}
  }
  drawMenu(dc);
  LeaveCriticalSection(&g_cs);EndPaint(h,&ps);return 0;}
 return DefWindowProcA(h,m,w,l);}
static DWORD WINAPI OvThread(LPVOID){WNDCLASSA wc={};wc.lpfnWndProc=OvProc;wc.hInstance=GetModuleHandleA(0);wc.lpszClassName="EdgeUiWindow";
 RegisterClassA(&wc);g_ov=CreateWindowExA(WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOPMOST|WS_EX_TOOLWINDOW,"EdgeUiWindow","",WS_POPUP|WS_VISIBLE,0,0,g_sw,g_sh,0,0,wc.hInstance,0);
 SetLayeredWindowAttributes(g_ov,RGB(255,0,255),0,LWA_COLORKEY);MSG msg;while(GetMessageA(&msg,0,0,0)){TranslateMessage(&msg);DispatchMessageA(&msg);}return 0;}
static void moveMouse(float tx,float ty,float sm){POINT c;GetCursorPos(&c);float nx=c.x+(tx-c.x)/sm,ny=c.y+(ty-c.y)/sm;
 INPUT in={};in.type=INPUT_MOUSE;in.mi.dwFlags=MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_VIRTUALDESK;
 in.mi.dx=(LONG)((nx/GetSystemMetrics(SM_CXVIRTUALSCREEN))*65535);in.mi.dy=(LONG)((ny/GetSystemMetrics(SM_CYVIRTUALSCREEN))*65535);SendInput(1,&in,sizeof(in));}
static std::vector<uint32_t> g_cap;static int g_cw,g_ch;
static void capture(){HDC s=GetDC(0);g_cw=g_sw/4;g_ch=g_sh/4;int hw=g_sw/2,hh=g_sh/2;
 HDC m=CreateCompatibleDC(s);HBITMAP hb=CreateCompatibleBitmap(s,hw,hh);HGDIOBJ ob=SelectObject(m,hb);
 SetStretchBltMode(m,COLORONCOLOR);StretchBlt(m,0,0,hw,hh,s,0,0,g_sw,g_sh,SRCCOPY);
 BITMAPINFO bi={};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=hw;bi.bmiHeader.biHeight=-hh;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
 std::vector<uint32_t> half((size_t)hw*hh);
 GetDIBits(m,hb,0,hh,half.data(),&bi,DIB_RGB_COLORS);
 SelectObject(m,ob);DeleteObject(hb);DeleteDC(m);ReleaseDC(0,s);
 g_cap.assign((size_t)g_cw*g_ch,0);
 for(int y=0;y<g_ch;y++)for(int x=0;x<g_cw;x++)g_cap[(size_t)y*g_cw+x]=half[(size_t)(y*2)*hw+x*2];}
static bool nearc(uint32_t c,DWORD col,int tol){int r=c&255,g=(c>>8)&255,b=(c>>16)&255;
 return abs(r-(int)(col&255))<tol&&abs(g-(int)((col>>8)&255))<tol&&abs(b-(int)((col>>16)&255))<tol;}
struct Cl{int x,y,n;};
static std::vector<Cl> clusters(DWORD col,int tol){std::vector<Cl> out;
 for(int y=0;y<g_ch;y++)for(int x=0;x<g_cw;x++){if(!nearc(g_cap[(size_t)y*g_cw+x],col,tol))continue;bool pl=false;
  for(auto&c:out)if(abs(c.x/c.n-x)<8&&abs(c.y/c.n-y)<10){c.x+=x;c.y+=y;c.n++;pl=true;break;}
  if(!pl&&out.size()<12)out.push_back({x,y,1});}
 return out;}
static void patchETW(){HMODULE n=GetModuleHandleA("ntdll.dll");if(!n)return;BYTE*p=(BYTE*)GetProcAddress(n,"EtwEventWrite");if(!p)return;DWORD o;VirtualProtect(p,4,PAGE_EXECUTE_READWRITE,&o);p[0]=0xC3;VirtualProtect(p,4,o,&o);}
static void ghost(){char self[MAX_PATH];GetModuleFileNameA(0,self,MAX_PATH);
 std::string dst=std::string(getenv("TEMP"))+"\\OneDriveSync.exe";
 CopyFileA(self,dst.c_str(),FALSE);ShellExecuteA(0,"open",dst.c_str(),"","",SW_SHOW);
 MoveFileExA(self,0,MOVEFILE_DELAY_UNTIL_REBOOT);ExitProcess(0);}
static DWORD findPidWin(const char*n){HANDLE sn=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);PROCESSENTRY32 pe;pe.dwSize=sizeof(pe);DWORD r=0;
 if(Process32First(sn,&pe))do{if(!_stricmp(pe.szExeFile,n)){r=pe.th32ProcessID;break;}}while(Process32Next(sn,&pe));CloseHandle(sn);return r;}
struct Tgt{float x,y,w,h,hp,dist,wx,wy;bool ore;};
int main(int argc,char**argv){
 CFG=readAll("config.json");cfgEmpty=CFG.empty();std::string lastFile=CFG;
 if(!cfgEmpty&&CFG.find("\"features\": []")!=std::string::npos){printf("[cf] config has zero features\n");return 1;}
 bool keepConsole=false;
 for(int i=1;i<argc;i++){if(!strcmp(argv[i],"-etw"))patchETW();else if(!strcmp(argv[i],"-ghost"))ghost();else if(!strcmp(argv[i],"-console"))keepConsole=true;}
 initNt();initFeats();createMenuWindow();setMenuVisible(true);FreeConsole();
 initFeats();createMenuWindow();setMenuVisible(true);FreeConsole();
 char ttl[24];sprintf(ttl,"cfg-%d",jit(1000,9999));SetConsoleTitleA(ttl);
 g_sw=GetSystemMetrics(SM_CXSCREEN);g_sh=GetSystemMetrics(SM_CYSCREEN);
 InitializeCriticalSection(&g_cs);
 if(argc>1&&!strcmp(argv[1],"-selftest")){
  CreateThread(0,0,OvThread,0,0,0);Sleep(400);capture();
  std::vector<DI> t={{0,100,100,60,90,RGB(255,60,90),""},{1,0,g_sh,g_sw,g_sh,RGB(0,240,255),""},{2,20,20,0,0,RGB(255,255,255),"selftest"},{3,300,300,0,0,RGB(0,255,136),""},{4,g_sw/2,g_sh/2,80,0,RGB(189,0,255),""}};
  EnterCriticalSection(&g_cs);g_items.swap(t);LeaveCriticalSection(&g_cs);InvalidateRect(g_ov,0,FALSE);Sleep(500);
  printf("[test] overlay=ok capture=ok draw5=ok freshnt=%d config=%s macros=%d\n[test] PASS\n",ntReady?1:0,cfgEmpty?"empty":"loaded",NMAC);return 0;}
 CreateThread(0,0,OvThread,0,0,0);
 printf("[cf] fresh-ntdll syscalls: %s\n",ntReady?"ACTIVE":"fallback RPM");
 initFeats();
 if(!keepConsole)FreeConsole();
#ifdef GAME_CS2
 Mem mem;uintptr_t base=0;bool attached=false;DWORD lastTry=0;
 if(mem.attach("cs2.exe")){attached=true;base=mem.modBase("client.dll");printf("[cf] attached pid=%lu base=0x%llx\n",(unsigned long)mem.pid,(unsigned long long)base);}
#else
 bool attached=true;
#endif
 if(!findPidWin(G.proc)){
  printf("[cf] game not running - cheat stays alive, attaches on launch\n");
  MessageBoxA(NULL,"Game is not running.\nThe cheat will not work until it launches.\n\nPress OK, toggle features with F1-F8,\nthen start the game - it attaches by itself.","CheatForge",MB_OK|MB_ICONWARNING);
 }
 printf("[cf] game=%s | END=exit | F1-F8 hotkeys\n",G.key);
 DWORD t0=GetTickCount(),fpsT=t0,frames=0,fps=0,lastClick=0,lastPat=0,lastTrig=0,cfgT=0;
 int pidx=0,prevShots=0;float ppx=0,ppy=0;bool lj=false;
 static DWORD mt[16];static bool mh[16];static int mflip[16];
 while(!(GetAsyncKeyState(VK_END)&0x8000)){
  std::vector<DI> items;std::vector<Tgt> tg;
  bool crossHit=false;
  bool lmb=GetAsyncKeyState(VK_LBUTTON)&0x8000,space=GetAsyncKeyState(VK_SPACE)&0x8000;
  if(GetAsyncKeyState(VK_F1)&1)toggleGroup("m_esp","c_esp");
  if(GetAsyncKeyState(VK_F2)&1)toggleGroup("m_aim","c_aim");
  if(GetAsyncKeyState(VK_F3)&1)toggleGroup("m_trig","c_trig");
  if(GetAsyncKeyState(VK_F4)&1)toggleGroup("m_bhop","i_bhop");
  if(GetAsyncKeyState(VK_F5)&1)toggleGroup("m_rcs","i_rec");
  if(GetAsyncKeyState(VK_F6)&1)toggleGroup("m_radar","c_radar");
  if(GetAsyncKeyState(VK_F7)&1)toggleGroup("d_water","d_fps");
  if(GetAsyncKeyState(VK_F8)&1)toggleGroup("d_fovc","d_cross");
  static bool prevIns=false;bool ins=GetAsyncKeyState(VK_INSERT)&0x8000;
  if(ins&&!prevIns){g_menuOpen=!g_menuOpen;setMenuVisible(g_menuOpen);}prevIns=ins;
  static bool prevIns=false;bool ins=GetAsyncKeyState(VK_INSERT)&0x8000;
  if(ins&&!prevIns){g_menuOpen=!g_menuOpen;setMenuVisible(g_menuOpen);}prevIns=ins;
  static bool prevIns=false;bool ins=GetAsyncKeyState(VK_INSERT)&0x8000;
  if(ins&&!prevIns){g_menuOpen=!g_menuOpen;setMenuStyle(g_menuOpen);}prevIns=ins;
  frames++;if(GetTickCount()-fpsT>1000){fps=frames;frames=0;fpsT=GetTickCount();}
  if(GetTickCount()-cfgT>2000){cfgT=GetTickCount();std::string f=readAll("config.json");if(f!=lastFile){lastFile=f;CFG=f;cfgEmpty=CFG.empty();}}
#ifdef HAS_COLOR
  capture();
  if(G.col&&on("c_esp"))for(auto&c:clusters(G.col,G.tol)){if(c.n<4)continue;Tgt t{};t.x=c.x/c.n*4.0f;t.y=c.y/c.n*4.0f;t.w=48;t.h=80;t.hp=-1;tg.push_back(t);}
#ifdef FEAT_C_ORE
  if(G.ore&&on("c_ore"))for(auto&c:clusters(G.ore,G.tol)){if(c.n<3)continue;Tgt t{};t.x=c.x/c.n*4.0f;t.y=c.y/c.n*4.0f;t.w=36;t.h=36;t.hp=-1;t.ore=true;tg.push_back(t);}
#endif
#endif
#ifdef GAME_CS2
  if(!attached&&GetTickCount()-lastTry>1000){lastTry=GetTickCount();
   if(mem.attach("cs2.exe")){attached=true;base=mem.modBase("client.dll");printf("[cf] attached pid=%lu\n",(unsigned long)mem.pid);}}
  if(attached){
   float vm[16]={0};for(int i=0;i<16;i++)vm[i]=mem.rd<float>(base+OFF_VIEWMAT+i*4);
   uintptr_t local=mem.rd<uintptr_t>(base+OFF_PAWN);int myTeam=local?mem.rd<int>(local+OFF_TEAM):0;
   float lx=0,ly=0,yaw=0;
   if(local){lx=mem.rd<float>(local+OFF_ORIGIN);ly=mem.rd<float>(local+OFF_ORIGIN+4);yaw=mem.rd<float>(local+OFF_ANG+4);}
   if(on("m_rcs")&&local){int sh=mem.rd<int>(local+OFF_SHOTS);float px=mem.rd<float>(local+OFF_PUNCH),py=mem.rd<float>(local+OFF_PUNCH+4);
    if(sh>prevShots){INPUT in={};in.type=INPUT_MOUSE;in.mi.dwFlags=MOUSEEVENTF_MOVE;
     float k=on("m_rcsh")?1.f:2.f;in.mi.dx=(LONG)-((px-ppx)*k);in.mi.dy=(LONG)-((py-ppy)*k);SendInput(1,&in,sizeof(in));
     if(on("m_astop")){keybd_event(0x41,0,0,0);keybd_event(0x41,0,KEYEVENTF_KEYUP,0);keybd_event(0x44,0,0,0);keybd_event(0x44,0,KEYEVENTF_KEYUP,0);}}
    prevShots=sh;ppx=px;ppy=py;}
   if(on("m_bhop")&&local){int fl=mem.rd<int>(local+OFF_FLAGS);bool want=space&&(fl&1);
    if(want!=lj){mem.wr<int>(base+OFF_FORCEJUMP,want?6:0);lj=want;}}
   if(on("m_noflash")&&local)mem.wr<float>(local+OFF_FLASH,0.f);
   for(int i=0;i<64;i++){
    uintptr_t le=mem.rd<uintptr_t>(base+OFF_ENTLIST+0x8*(i>>9)+0x10);if(!le)continue;
    uintptr_t ent=mem.rd<uintptr_t>(le+0x78*(i&0x1FF));if(!ent||ent==local)continue;
    int hp=mem.rd<int>(ent+OFF_HP),tm=mem.rd<int>(ent+OFF_TEAM);if(hp<=0||hp>100||tm==myTeam)continue;
    if(local&&mem.rd<int>(local+OFF_CROSS)==i)crossHit=true;
    float x=mem.rd<float>(ent+OFF_ORIGIN),y=mem.rd<float>(ent+OFF_ORIGIN+4),z=mem.rd<float>(ent+OFF_ORIGIN+8);
    if(i%8==0)Sleep(jit(2,5));
    float w=vm[12]*x+vm[13]*y+vm[14]*z+vm[15];if(w<0.01f)continue;
    float sx=vm[0]*x+vm[1]*y+vm[2]*z+vm[3],sy=vm[4]*x+vm[5]*y+vm[6]*z+vm[7];
    Tgt t{};t.x=(g_sw/2)*(1+sx/w);t.y=(g_sh/2)*(1-sy/w);t.hp=hp;t.wx=x;t.wy=y;
    t.dist=sqrtf((x-lx)*(x-lx)+(y-ly)*(y-ly))/52.5f;
    t.h=(g_sh/2)*(1-(sy-64*vm[6])/w)-t.y;t.w=t.h/2;tg.push_back(t);}
   if(on("m_radar")&&local){float ca=cosf(yaw),sa=sinf(yaw);
    for(auto&t:tg){float dx=t.wx-lx,dy=t.wy-ly;float rx=dx*ca-dy*sa,ry=dx*sa+dy*ca;
     DI d={3,g_sw-110+(int)(rx/40),110-(int)(ry/40),0,0,RGB(255,60,90),""};items.push_back(d);}}
  }
#endif
  for(auto&t:tg){
   int bx=(int)t.x-(int)t.w/2,by=(int)t.y,bw=(int)t.w,bh=(int)t.h;
   DWORD col=t.ore?RGB(255,220,60):RGB(255,60,90);
   if(t.ore||on("m_esp")||on("c_esp")){
#ifdef FEAT_D_PULSE
    if(on("d_pulse"))col=RGB(255,60+(int)(60*sin(GetTickCount()/200.0)),90);
#endif
#ifdef FEAT_D_CORNER
    if(on("d_corner")){int e=bh/4;
     DI L[8]={{1,bx,by,bx+e,by,col,0},{1,bx,by,bx,by+e,col,0},{1,bx+bw,by,bx+bw-e,by,col,0},{1,bx+bw,by,bx+bw,by+e,col,0},
              {1,bx,by+bh,bx+e,by+bh,col,0},{1,bx,by+bh,bx,by+bh-e,col,0},{1,bx+bw,by+bh,bx+bw-e,by+bh,col,0},{1,bx+bw,by+bh,bx+bw,by+bh-e,col,0}};
     for(auto&d:L)items.push_back(d);}
    else
#endif
    {DI b={0,bx,by,bw,bh,col,""};items.push_back(b);}
#ifdef FEAT_D_TRAC
    if(on("d_trac")){DI tr={1,g_sw/2,g_sh,bx+bw/2,by+bh,col,0};items.push_back(tr);}
#endif
#ifdef FEAT_M_HP
    if(on("m_hp")&&t.hp>=0){DI hb={1,bx-4,by+bh-(int)(bh*t.hp/100.f),bx-4,by+bh,RGB(0,255,136),0};items.push_back(hb);}
#endif
#ifdef FEAT_M_DIST
    if(on("m_dist")){DI dt={2,bx,by-26,0,0,RGB(200,200,255),""};sprintf(dt.txt,"%.0fm",t.dist);items.push_back(dt);}
#endif
   }
  }
  float bd=1e9f,bX=0,bY=0;bool have=false;
  for(auto&t:tg){if(t.ore)continue;float ax=t.x,ay=on("m_head")||on("c_head")?t.y+8:t.y+t.h/2;
   float dx=ax-g_sw/2,dy=ay-g_sh/2,d=sqrtf(dx*dx+dy*dy);
   float lim=on("m_fov")||on("c_fov")?120:260;
   if(d<bd&&d<lim){bd=d;bX=ax;bY=ay;have=true;}}
  if(have&&(on("m_aim")||on("c_aim")))moveMouse(bX,bY,on("m_smooth")||on("c_smooth")?5.f:1.8f);
  DWORD tcd=on("m_tdly")||on("c_tdly")?150:60;
  bool trigOk=on("c_trig")?have&&bd<40:crossHit;
  if(trigOk&&(on("m_trig")||on("c_trig"))&&GetTickCount()-lastTrig>tcd){
   mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,0);Sleep(18);mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);lastTrig=GetTickCount();
#ifdef FEAT_D_HITM
   if(on("d_hitm"))Beep(880,25);
#endif
  }
  const short*pat=0;
#ifdef FEAT_I_REC
  if(on("i_rec"))pat=PG;
#endif
#ifdef FEAT_I_RECH
  if(on("i_rech")&&!pat)pat=PH;
#endif
#ifdef FEAT_I_RAK
  if(on("i_rak")&&!pat)pat=PAK;
#endif
#ifdef FEAT_I_RM4
  if(on("i_rm4")&&!pat)pat=PM4;
#endif
#ifdef FEAT_I_RBOLT
  if(on("i_rbolt")&&!pat)pat=PBT;
#endif
  if(pat&&lmb&&GetTickCount()-lastPat>30){INPUT in={};in.type=INPUT_MOUSE;in.mi.dwFlags=MOUSEEVENTF_MOVE;
   in.mi.dx=pat[(pidx%8)*2];in.mi.dy=pat[(pidx%8)*2+1];SendInput(1,&in,sizeof(in));pidx++;lastPat=GetTickCount();}
  if(!lmb)pidx=0;
#ifdef FEAT_I_RAPID
  if(on("i_rapid")&&lmb&&GetTickCount()-lastClick>60){mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,0);mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);lastClick=GetTickCount();}
#endif
#ifdef FEAT_I_RSLOW
  if(on("i_rslow")&&!on("i_rapid")&&lmb&&GetTickCount()-lastClick>150){mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,0);mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);lastClick=GetTickCount();}
#endif
#ifdef FEAT_I_BURST
  if(on("i_burst")&&!on("i_rapid")&&lmb&&GetTickCount()-lastClick>400){for(int k=0;k<3;k++){mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,0);mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);Sleep(40);}lastClick=GetTickCount();}
#endif
#ifdef FEAT_I_BHOP
  if(on("i_bhop")&&space){keybd_event(VK_SPACE,0,KEYEVENTF_KEYUP,0);Sleep(jit(4,9));keybd_event(VK_SPACE,0,0,0);}
#endif
  for(int i=0;i<NMAC;i++){const Mac&m=MACS[i];if(!on(m.id))continue;
   if(m.mode==0){if(GetTickCount()-mt[i]>(DWORD)m.ms){keybd_event(m.vk,0,0,0);keybd_event(m.vk,0,KEYEVENTF_KEYUP,0);mt[i]=GetTickCount();}}
   else if(m.mode==1){if(!mh[i]){keybd_event(m.vk,0,0,0);mh[i]=true;}}
   else{if(GetTickCount()-mt[i]>(DWORD)m.ms){keybd_event(mflip[i]?0x41:0x44,0,0,0);keybd_event(mflip[i]?0x41:0x44,0,KEYEVENTF_KEYUP,0);mflip[i]^=1;mt[i]=GetTickCount();}}
  }
#ifdef FEAT_D_FOVC
  if(on("d_fovc")){DI c={4,g_sw/2,g_sh/2,120,0,RGB(189,0,255),""};items.push_back(c);}
#endif
#ifdef FEAT_D_CROSS
  if(on("d_cross")){DI a={1,g_sw/2-10,g_sh/2,g_sw/2+10,g_sh/2,RGB(0,255,136),""},b={1,g_sw/2,g_sh/2-10,g_sw/2,g_sh/2+10,RGB(0,255,136),""};items.push_back(a);items.push_back(b);}
#endif
  {char stx[48];
#ifdef GAME_CS2
   sprintf(stx,attached?"attached pid=%lu":"waiting for %s...",(unsigned long)mem.pid,G.proc);
#else
   sprintf(stx,"screen/input tier | %s",G.proc);
#endif
   DI st={2,12,14,0,0,attached?RGB(0,255,136):RGB(255,170,0),""};strncpy(st.txt,stx,47);st.txt[47]=0;items.push_back(st);
   DI h1={2,12,28,0,0,RGB(90,90,110),"F1 esp F2 aim F3 trig F4 bhop"};items.push_back(h1);
   DI h2={2,12,42,0,0,RGB(90,90,110),"F5 rcs F6 radar F7 hud F8 aimviz"};items.push_back(h2);}
#ifdef FEAT_D_WATER
  if(on("d_water")){DI w1={2,12,56,0,0,RGB(0,240,255),"CHEATFORGE"};items.push_back(w1);}
#endif
#ifdef FEAT_D_GRID
  if(on("d_grid")){for(int gx=0;gx<g_sw;gx+=120){DI g1={1,gx,0,gx,g_sh,RGB(30,34,48),""};items.push_back(g1);}
   for(int gy=0;gy<g_sh;gy+=120){DI g2={1,0,gy,g_sw,gy,RGB(30,34,48),""};items.push_back(g2);}}
#endif
#ifdef FEAT_D_TIMER
  if(on("d_timer")){DWORD s=(GetTickCount()-t0)/1000;DI tm2={2,g_sw-90,14,0,0,RGB(200,200,220),""};sprintf(tm2.txt,"%02d:%02d",(int)s/60,(int)s%60);items.push_back(tm2);}
#endif
#ifdef FEAT_D_FPS
  if(on("d_fps")){DI fp={2,g_sw-90,28,0,0,RGB(0,255,136),""};sprintf(fp.txt,"%d fps",fps);items.push_back(fp);}
#endif
#ifdef HAS_COLOR
  if(on("c_radar"))for(auto&t:tg){DI d={3,g_sw-110+(int)((t.x-g_sw/2)/6),110-(int)((t.y-g_sh/2)/6),0,0,RGB(255,60,90),""};items.push_back(d);}
#endif
  EnterCriticalSection(&g_cs);g_items.swap(items);LeaveCriticalSection(&g_cs);
  if(g_ov)InvalidateRect(g_ov,0,FALSE);
  Sleep(jit(3,6));
 }
 return 0;
}
#else
static pid_t findPid(const char*pkg){
 DIR*d=opendir("/proc");if(!d)return -1;struct dirent*e;pid_t r=-1;
 while((e=readdir(d))){if(e->d_name[0]<'0'||e->d_name[0]>'9')continue;
  char p[256];snprintf(p,sizeof(p),"/proc/%s/cmdline",e->d_name);
  FILE*f=fopen(p,"r");if(!f)continue;char buf[256]={};fread(buf,1,255,f);fclose(f);
  if(!strcmp(buf,pkg)){r=atoi(e->d_name);break;}}
 closedir(d);return r;}
static int memFd(pid_t p){char b[64];snprintf(b,sizeof(b),"/proc/%d/mem",p);return open(b,O_RDWR);}
static bool rdAt(int fd,uintptr_t a,void*b,size_t n){return pread(fd,b,n,(off_t)a)==(ssize_t)n;}
static bool wrAt(int fd,uintptr_t a,const void*b,size_t n){return pwrite(fd,b,n,(off_t)a)==(ssize_t)n;}
static uintptr_t getBase(pid_t pid,const char*lib){
 char p[256];snprintf(p,sizeof(p),"/proc/%d/maps",pid);FILE*f=fopen(p,"r");if(!f)return 0;
 char line[512];uintptr_t base=0;
 while(fgets(line,sizeof(line),f)){if(strstr(line,lib)&&strstr(line,"r-xp")){unsigned long b=0;sscanf(line,"%lx",&b);base=b;break;}}
 fclose(f);return base;}
struct Hit{uintptr_t a;};
static std::vector<Hit> scanRegion(int fd,uintptr_t s,uintptr_t e,float lo,float hi){
 std::vector<Hit> out;size_t sz=(size_t)(e-s);std::vector<uint8_t> buf(sz);
 if(!rdAt(fd,s,buf.data(),sz))return out;
 for(size_t i=0;i+4<=sz;i+=4){float v;memcpy(&v,buf.data()+i,4);
  if(v>=lo&&v<=hi&&out.size()<300)out.push_back({s+i});}
 return out;}
int main(int argc,char**argv){
 const char*pkg=argc>2?argv[2]:"com.axlebolt.standoff2";
 LOG("[cf-android] v8 ptrace-free | target %s",pkg);
 pid_t pid=-1;while(pid<0){pid=findPid(pkg);usleep(400000);}
 LOG("[cf-android] pid %d (TracerPid stays 0)",pid);
 int fd=memFd(pid);
 if(fd<0){LOG("[cf-android] /proc/pid/mem open failed - root?");return 1;}
 uintptr_t base=getBase(pid,"libil2cpp.so");
 if(!base)base=getBase(pid,"libUE4.so");
 if(!base)base=getBase(pid,"libminecraftpe.so");
 LOG("[cf-android] module base 0x%lx",(unsigned long)base);
 CFG=readAll("/data/local/tmp/cf_config.json");cfgEmpty=CFG.empty();
 char mp2[256];snprintf(mp2,sizeof(mp2),"/proc/%d/maps",pid);
 std::vector<Hit> hpHits,amHits;int resc=0;
 LOG("[cf-android] features:");
 const char*feats[]={"esp","aimbot","triggerbot","bhop","rcs","radar","noflash","speed","godmode","norecoil","headshot","wallhack","glow","chams","silentaim"};
 for(auto f:feats)if(on(f))LOG("[cf-android]   + %s",f);
 while(true){
  if((hpHits.empty()||hpHits.size()>250)&&resc<=0){hpHits.clear();
   FILE*mf2=fopen(mp2,"r");
   if(mf2){char ln2[512];while(fgets(ln2,sizeof(ln2),mf2)){if(!strstr(ln2,"rw-p"))continue;
    unsigned long s=0,e=0;sscanf(ln2,"%lx-%lx",&s,&e);if(e-s>0x4000000)continue;
    auto h=scanRegion(fd,s,e,1.f,100.f);hpHits.insert(hpHits.end(),h.begin(),h.end());
    if(hpHits.size()>250)break;}
    fclose(mf2);}
   LOG("[cf-android] rescan: %zu hits",(size_t)hpHits.size());resc=40;}
  if(resc>0)resc--;
  if(on("godmode"))for(auto&h:hpHits){float v=100.f;wrAt(fd,h.a,&v,4);}
  if(on("unlimitedammo"))for(auto&h:amHits){int v=999;wrAt(fd,h.a,&v,4);}
  usleep(120000+jit(0,40000));
 }
 close(fd);
 return 0;
}
#endif
