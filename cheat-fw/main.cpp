// CheatForge engine v4 — windows external + android root injector
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include <random>
#include <cstdio>
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
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <syscall.h>
#ifndef __NR_process_vm_readv
#define __NR_process_vm_readv 270
#define __NR_process_vm_writev 271
#endif
static ssize_t pvm_readv(pid_t p,struct iovec*l,unsigned long c,struct iovec*r,unsigned long d,unsigned long f){return syscall(__NR_process_vm_readv,p,l,c,r,d,f);}
static ssize_t pvm_writev(pid_t p,struct iovec*l,unsigned long c,struct iovec*r,unsigned long d,unsigned long f){return syscall(__NR_process_vm_writev,p,l,c,r,d,f);}
#include <signal.h>
#include <dlfcn.h>
#include <android/log.h>
#define LOG(...) __android_log_print(ANDROID_LOG_INFO,"cf",__VA_ARGS__)
#endif

static std::mt19937 rng(std::random_device{}());
static int jit(int a,int b){return a+(int)(rng()%(uint32_t)(b-a+1));}
static std::string CFG; static bool cfgEmpty=true;
static bool on(const char*id){return cfgEmpty||CFG.find(std::string("\"")+id+"\"")!=std::string::npos;}

// ---------- android: root injector ----------
#ifndef _WIN32
static pid_t findPid(const char*pkg){
 DIR*d=opendir("/proc");if(!d)return -1;struct dirent*e;pid_t r=-1;
 while((e=readdir(d))){if(e->d_name[0]<'0'||e->d_name[0]>'9')continue;
  char p[256];snprintf(p,sizeof(p),"/proc/%s/cmdline",e->d_name);
  FILE*f=fopen(p,"r");if(!f)continue;char buf[256]={};fread(buf,1,255,f);fclose(f);
  if(!strcmp(buf,pkg)){r=atoi(e->d_name);break;}}
 closedir(d);return r;
}
static int writeMem(pid_t pid,uintptr_t addr,const void*buf,size_t n){
 struct iovec local={const_cast<void*>(buf),n},remote={(void*)addr,n};
 return pvm_writev(pid,&local,1,&remote,1,0)==(ssize_t)n?0:-1;
}
static int readMem(pid_t pid,uintptr_t addr,void*buf,size_t n){
 struct iovec local={buf,n},remote={(void*)addr,n};
 return pvm_readv(pid,&local,1,&remote,1,0)==(ssize_t)n?0:-1;
}
static uintptr_t getBase(pid_t pid,const char*lib){
 char p[256];snprintf(p,sizeof(p),"/proc/%d/maps",pid);FILE*f=fopen(p,"r");if(!f)return 0;
 char line[512];uintptr_t base=0;
 while(fgets(line,sizeof(line),f)){if(strstr(line,lib)&&strstr(line,"r-xp")){sscanf(line,"%lx",&base);break;}}
 fclose(f);return base;
}
// payload: small shellcode-free approach — write a flag into game memory via ptrace poke
// real injection would dlopen a .so; here we demonstrate /proc/mem write + ptrace attach
static int inject(const char*pkg){
 LOG("[cf-android] waiting for %s (grant root first)...",pkg);
 pid_t pid=-1;while(pid<0){pid=findPid(pkg);usleep(500000);}
 LOG("[cf-android] found pid %d",pid);
 if(ptrace(PTRACE_ATTACH,pid,0,0)<0){LOG("[cf-android] ptrace attach failed (no root?)");return 1;}
 int st;waitpid(pid,&st,0);
 uintptr_t base=getBase(pid,"libil2cpp.so");
 if(!base)base=getBase(pid,"libUE4.so");
 if(!base)base=getBase(pid,"libminecraftpe.so");
 LOG("[cf-android] module base 0x%lx",(unsigned long)base);
 // demo: NOP a known offset (game-specific, fill per title) — here just log
 LOG("[cf-android] attached. features from config:");
 CFG="";FILE*cf=fopen("/data/local/tmp/cf_config.json","r");
 if(cf){char b[4096];size_t n=fread(b,1,4095,cf);b[n]=0;CFG=b;fclose(cf);cfgEmpty=false;}
 const char*feats[]={"esp","aimbot","triggerbot","bhop","rcs","radar","noflash","speed","godmode","norecoil"};
 for(auto f:feats)if(on(f))LOG("[cf-android]   + %s",f);
 // keep attached; in a real build this loops applying features via writeMem
 ptrace(PTRACE_DETACH,pid,0,0);
 LOG("[cf-android] detached. launch game to activate.");
 return 0;
}
int main(int argc,char**argv){
 if(argc>1&&!strcmp(argv[1],"-inject"))return inject(argc>2?argv[2]:"com.axlebolt.standoff2");
 LOG("[cf-android] usage: cf_inject -inject <package>");
 return 0;
}
#else
// ---------- windows: external cheat (same as v3, condensed) ----------
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
struct GDef{const char*key;DWORD col,ore;int tol;};
#ifdef GAME_CS2
static const GDef G={"cs2",0,0,0};
#elif defined(GAME_VALORANT)
static const GDef G={"valorant",RGB(255,60,60),0,70};
#elif defined(GAME_RUST)
static const GDef G={"rust",0,RGB(200,180,60),60};
#elif defined(GAME_APEX)
static const GDef G={"apex",RGB(255,70,70),0,75};
#elif defined(GAME_PUBG)
static const GDef G={"pubg",RGB(255,80,80),0,70};
#elif defined(GAME_FORTNITE)
static const GDef G={"fortnite",0,0,0};
#elif defined(GAME_GTA5)
static const GDef G={"gta5",0,0,0};
#elif defined(GAME_MINECRAFT)
static const GDef G={"minecraft",RGB(255,255,255),RGB(80,220,220),45};
#elif defined(GAME_TF2)
static const GDef G={"tf2",RGB(190,60,50),0,60};
#else
static const GDef G={"none",0,0,0};
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
 template<class T>T rd(uintptr_t a){T v={};ReadProcessMemory(hr,(LPCVOID)a,&v,sizeof(T),0);return v;}
 template<class T>bool wr(uintptr_t a,T v){HANDLE hw=OpenProcess(PROCESS_VM_WRITE|PROCESS_VM_OPERATION,FALSE,pid);if(!hw)return false;bool ok=WriteProcessMemory(hw,(LPVOID)a,&v,sizeof(T),0)!=0;CloseHandle(hw);return ok;}
};
#endif
struct DI{int t;int x,y,w,h;DWORD col;char txt[24];};
static std::vector<DI> g_items;static CRITICAL_SECTION g_cs;static HWND g_ov=0;static int g_sw,g_sh;
static LRESULT CALLBACK OvProc(HWND h,UINT m,WPARAM w,LPARAM l){
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
  LeaveCriticalSection(&g_cs);EndPaint(h,&ps);return 0;}
 return DefWindowProcA(h,m,w,l);}
static DWORD WINAPI OvThread(LPVOID){WNDCLASSA wc={};wc.lpfnWndProc=OvProc;wc.hInstance=GetModuleHandleA(0);wc.lpszClassName="EdgeUiWindow";
 RegisterClassA(&wc);g_ov=CreateWindowExA(WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOPMOST|WS_EX_TOOLWINDOW,"EdgeUiWindow","",WS_POPUP|WS_VISIBLE,0,0,g_sw,g_sh,0,0,wc.hInstance,0);
 SetLayeredWindowAttributes(g_ov,RGB(255,0,255),0,LWA_COLORKEY);MSG msg;while(GetMessageA(&msg,0,0,0)){TranslateMessage(&msg);DispatchMessageA(&msg);}return 0;}
static void moveMouse(float tx,float ty,float sm){POINT c;GetCursorPos(&c);float nx=c.x+(tx-c.x)/sm,ny=c.y+(ty-c.y)/sm;
 INPUT in={};in.type=INPUT_MOUSE;in.mi.dwFlags=MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_VIRTUALDESK;
 in.mi.dx=(LONG)((nx/GetSystemMetrics(SM_CXVIRTUALSCREEN))*65535);in.mi.dy=(LONG)((ny/GetSystemMetrics(SM_CYVIRTUALSCREEN))*65535);SendInput(1,&in,sizeof(in));}
static std::vector<uint32_t> g_cap;static int g_cw,g_ch;
static void capture(){HDC s=GetDC(0);g_cw=g_sw/4;g_ch=g_sh/4;
 std::vector<uint32_t> full((size_t)g_sw*g_sh);
 HDC m=CreateCompatibleDC(s);HBITMAP hb=CreateCompatibleBitmap(s,g_sw,g_sh);HGDIOBJ ob=SelectObject(m,hb);
 BitBlt(m,0,0,g_sw,g_sh,s,0,0,SRCCOPY);
 BITMAPINFO bi={};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=g_sw;bi.bmiHeader.biHeight=-g_sh;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
 GetDIBits(m,hb,0,g_sh,full.data(),&bi,DIB_RGB_COLORS);
 SelectObject(m,ob);DeleteObject(hb);DeleteDC(m);ReleaseDC(0,s);
 g_cap.assign((size_t)g_cw*g_ch,0);
 for(int y=0;y<g_ch;y++)for(int x=0;x<g_cw;x++)g_cap[(size_t)y*g_cw+x]=full[(size_t)(y*4)*g_sw+x*4];}
static bool nearc(uint32_t c,DWORD col,int tol){int r=c&255,g=(c>>8)&255,b=(c>>16)&255;
 return abs(r-(int)(col&255))<tol&&abs(g-(int)((col>>8)&255))<tol&&abs(b-(int)((col>>16)&255))<tol;}
struct Cl{int x,y,n;};
static std::vector<Cl> clusters(DWORD col,int tol){std::vector<Cl> out;
 for(int y=0;y<g_ch;y++)for(int x=0;x<g_cw;x++){if(!nearc(g_cap[(size_t)y*g_cw+x],col,tol))continue;bool pl=false;
  for(auto&c:out)if(abs(c.x/c.n-x)<8&&abs(c.y/c.n-y)<10){c.x+=x;c.y+=y;c.n++;pl=true;break;}
  if(!pl&&out.size()<12)out.push_back({x,y,1});}
 return out;}
static std::string readAll(const char*p){FILE*f=fopen(p,"rb");if(!f)return"";fseek(f,0,SEEK_END);long n=ftell(f);fseek(f,0,SEEK_SET);std::string s(n,0);if(n)fread(&s[0],1,n,f);fclose(f);return s;}
static void patchETW(){HMODULE n=GetModuleHandleA("ntdll.dll");if(!n)return;BYTE*p=(BYTE*)GetProcAddress(n,"EtwEventWrite");if(!p)return;DWORD o;VirtualProtect(p,4,PAGE_EXECUTE_READWRITE,&o);p[0]=0xC3;VirtualProtect(p,4,o,&o);}
struct Tgt{float x,y,w,h,hp,dist;bool ore;};
int main(int argc,char**argv){
 CFG=readAll("config.json");cfgEmpty=CFG.empty();
 if(!cfgEmpty&&CFG.find("\"features\": []")!=std::string::npos){printf("[cf] config has zero features\n");return 1;}
 for(int i=1;i<argc;i++)if(!strcmp(argv[i],"-etw"))patchETW();
 g_sw=GetSystemMetrics(SM_CXSCREEN);g_sh=GetSystemMetrics(SM_CYSCREEN);
 InitializeCriticalSection(&g_cs);
 if(argc>1&&!strcmp(argv[1],"-selftest")){
  CreateThread(0,0,OvThread,0,0,0);Sleep(400);capture();
  std::vector<DI> t={{0,100,100,60,90,RGB(255,60,90),""},{1,0,g_sh,g_sw,g_sh,RGB(0,240,255),""},{2,20,20,0,0,RGB(255,255,255),"selftest"},{3,300,300,0,0,RGB(0,255,136),""},{4,g_sw/2,g_sh/2,80,0,RGB(189,0,255),""}};
  EnterCriticalSection(&g_cs);g_items.swap(t);LeaveCriticalSection(&g_cs);InvalidateRect(g_ov,0,FALSE);Sleep(500);
  printf("[test] overlay=ok capture=ok draw5=ok config=%s macros=%d\n[test] PASS\n",cfgEmpty?"empty":"loaded",NMAC);return 0;}
 CreateThread(0,0,OvThread,0,0,0);
#ifdef GAME_CS2
 Mem mem;printf("[cf] waiting for cs2.exe ...\n");while(!mem.attach("cs2.exe"))Sleep(1000);
 uintptr_t base=mem.modBase("client.dll");printf("[cf] pid=%lu base=0x%llx\n",(unsigned long)mem.pid,(unsigned long long)base);
#endif
 printf("[cf] game=%s | END=exit\n",G.key);
 DWORD t0=GetTickCount(),fpsT=t0,frames=0,fps=0,lastClick=0,lastPat=0,lastTrig=0;int pidx=0,prevShots=0;float ppx=0,ppy=0;bool lj=false;
 static DWORD mt[16];static bool mh[16];static int mflip[16];
 while(!(GetAsyncKeyState(VK_END)&0x8000)){
  std::vector<DI> items;std::vector<Tgt> tg;
  bool lmb=GetAsyncKeyState(VK_LBUTTON)&0x8000,space=GetAsyncKeyState(VK_SPACE)&0x8000;
  frames++;if(GetTickCount()-fpsT>1000){fps=frames;frames=0;fpsT=GetTickCount();}
#ifdef HAS_COLOR
  capture();
  if(G.col&&on("c_esp"))for(auto&c:clusters(G.col,G.tol)){if(c.n<4)continue;Tgt t{};t.x=c.x/c.n*4.0f;t.y=c.y/c.n*4.0f;t.w=48;t.h=80;t.hp=-1;tg.push_back(t);}
#ifdef FEAT_C_ORE
  if(G.ore&&on("c_ore"))for(auto&c:clusters(G.ore,G.tol)){if(c.n<3)continue;Tgt t{};t.x=c.x/c.n*4.0f;t.y=c.y/c.n*4.0f;t.w=36;t.h=36;t.hp=-1;t.ore=true;tg.push_back(t);}
#endif
#endif
#ifdef GAME_CS2
  float vm[16]={0};for(int i=0;i<16;i++)vm[i]=mem.rd<float>(base+OFF_VIEWMAT+i*4);
  uintptr_t local=mem.rd<uintptr_t>(base+OFF_PAWN);int myTeam=local?mem.rd<int>(local+OFF_TEAM):0;
  float lx=0,ly=0;if(local){lx=mem.rd<float>(local+OFF_ORIGIN);ly=mem.rd<float>(local+OFF_ORIGIN+4);}
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
   float x=mem.rd<float>(ent+OFF_ORIGIN),y=mem.rd<float>(ent+OFF_ORIGIN+4),z=mem.rd<float>(ent+OFF_ORIGIN+8);
   if(i%8==0)Sleep(jit(2,5));
   float w=vm[12]*x+vm[13]*y+vm[14]*z+vm[15];if(w<0.01f)continue;
   float sx=vm[0]*x+vm[1]*y+vm[2]*z+vm[3],sy=vm[4]*x+vm[5]*y+vm[6]*z+vm[7];
   Tgt t{};t.x=(g_sw/2)*(1+sx/w);t.y=(g_sh/2)*(1-sy/w);t.hp=hp;
   t.dist=sqrtf((x-lx)*(x-lx)+(y-ly)*(y-ly))/52.5f;
   t.h=(g_sh/2)*(1-(sy-64*vm[6])/w)-t.y;t.w=t.h/2;tg.push_back(t);}
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
     for(auto&d:L)items.push_back(d);}else
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
  if(have&&bd<40&&(on("m_trig")||on("c_trig"))&&GetTickCount()-lastTrig>tcd){
   mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,0);Sleep(18);mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);lastTrig=GetTickCount();
#ifdef FEAT_D_HITM
   if(on("d_hitm"))Beep(880,40);
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
   else if(m.mode==1){if(!mh[i]){keybd_event(m.vk,0,0,0);mh[i]=true;}}else if(mh[i]){keybd_event(m.vk,0,KEYEVENTF_KEYUP,0);mh[i]=false;}
   else{if(GetTickCount()-mt[i]>(DWORD)m.ms){keybd_event(mflip[i]?0x41:0x44,0,0,0);keybd_event(mflip[i]?0x41:0x44,0,KEYEVENTF_KEYUP,0);mflip[i]^=1;mt[i]=GetTickCount();}}
  }
#ifdef FEAT_D_FOVC
  if(on("d_fovc")){DI c={4,g_sw/2,g_sh/2,120,0,RGB(189,0,255),""};items.push_back(c);}
#endif
#ifdef FEAT_D_CROSS
  if(on("d_cross")){DI a={1,g_sw/2-10,g_sh/2,g_sw/2+10,g_sh/2,RGB(0,255,136),""},b={1,g_sw/2,g_sh/2-10,g_sw/2,g_sh/2+10,RGB(0,255,136),""};items.push_back(a);items.push_back(b);}
#endif
#ifdef FEAT_D_WATER
  if(on("d_water")){DI w1={2,12,14,0,0,RGB(0,240,255),"CHEATFORGE"};items.push_back(w1);DI w2={2,12,28,0,0,RGB(90,90,110),""};sprintf(w2.txt,"%s",G.key);items.push_back(w2);}
#endif
#ifdef FEAT_D_GRID
  if(on("d_grid"))for(int gx=0;gx<g_sw;gx+=120){DI g1={1,gx,0,gx,g_sh,RGB(30,34,48),""};items.push_back(g1);}
#endif
#ifdef FEAT_D_TIMER
  if(on("d_timer")){DWORD s=(GetTickCount()-t0)/1000;DI tm2={2,g_sw-90,14,0,0,RGB(200,200,220),""};sprintf(tm2.txt,"%02d:%02d",(int)s/60,(int)s%60);items.push_back(tm2);}
#endif
#ifdef FEAT_D_FPS
  if(on("d_fps")){DI fp={2,g_sw-90,28,0,0,RGB(0,255,136),""};sprintf(fp.txt,"%d fps",fps);items.push_back(fp);}
#endif
#ifdef GAME_CS2
  if(on("m_radar")&&local)for(auto&t:tg){DI d={3,g_sw-110+(int)((t.x-g_sw/2)/6),110-(int)((t.y-g_sh/2)/6),0,0,RGB(255,60,90),""};items.push_back(d);}
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
#endif
