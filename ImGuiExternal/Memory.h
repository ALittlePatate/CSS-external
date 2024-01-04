#include <iostream>
#include <Windows.h>
#include <string>
#include <sstream>
#include <iomanip>

#define ENTITY_LIST 0x004D5AE4 //client
#define LOCALPLAYER 0x004C88E8 //client
#define NAME_LIST 0x0050E368 //client
#define HEALTH 0x94 //player
#define XYZ 0x260 //player
#define TEAM 0x9C //player
#define DORMANT 0x17C //player
#define LIFESTATE 0x93 //player
#define BONEMATRIX 0x578 //player https://youtu.be/elKUMiqitxY
#define VIEW_MATRIX 0x5B0D68 //engine
#define MAP_NAME 0x47796D //engine

class Vector3 {
public:
    float x, y, z;
};

class Vector2 {
public:
    float x, y;
};

namespace Process {
	DWORD ID;
	HANDLE Handle;
	HWND Hwnd;
	WNDPROC WndProc;
	int WindowWidth;
	int WindowHeight;
	int WindowLeft;
	int WindowRight;
	int WindowTop;
	int WindowBottom;
	LPCSTR Title;
	LPCSTR ClassName;
	LPCSTR Path;
}

namespace Game {
    HANDLE handle = NULL;
    uintptr_t client = NULL;
    uintptr_t engine = NULL;
    char path[MAX_PATH];
    DWORD PID = 0;
}

template<typename TYPE>
TYPE RPM(DWORD address) {
    TYPE buffer;
    ReadProcessMemory(Game::handle, (LPCVOID)address, &buffer, sizeof(buffer), 0);
    return buffer;
}

float vmatrix_t[4][4];
bool WorldToScreen(const Vector3& vIn, Vector3& vOut)
{
    ReadProcessMemory(Game::handle, (void*)(Game::engine + VIEW_MATRIX), &vmatrix_t, sizeof(vmatrix_t), NULL);

    FLOAT w = vmatrix_t[3][0] * vIn.x + vmatrix_t[3][1] * vIn.y + vmatrix_t[3][2] * vIn.z + vmatrix_t[3][3];

    if (w < 0.01f)
        return false;

    vOut.x = vmatrix_t[0][0] * vIn.x + vmatrix_t[0][1] * vIn.y + vmatrix_t[0][2] * vIn.z + vmatrix_t[0][3];
    vOut.y = vmatrix_t[1][0] * vIn.x + vmatrix_t[1][1] * vIn.y + vmatrix_t[1][2] * vIn.z + vmatrix_t[1][3];
    FLOAT invw = 1.0f / w;

    vOut.x *= invw;
    vOut.y *= invw;

    int width = Process::WindowWidth;
    int height = Process::WindowHeight;

    float x = width / 2.0f;
    float y = height / 2.0f;

    x += 0.5f * vOut.x * width + 0.5f;
    y -= 0.5f * vOut.y * height + 0.5f;

    vOut.x = x;
    vOut.y = y;

    return true;
}

enum class BoneID {
    PELVIS,
    LEFT_HIPS,
    LEFT_KNEE,
    LEFT_ANKLE,
    LEFT_FOOT,
    RIGHT_HIPS,
    RIGHT_KNEE,
    RIGHT_ANKLE,
    RIGHT_FOOT,
    STOMACH,
    BODY,
    CHEST,
    NECK,
    FACE,
    HEAD,
    LEFT_NECK,
    LEFT_SHOULDER,
    LEFT_ELBOW,
    LEFT_HAND,
    RIGHT_NECK = 28,
    RIGHT_SHOULDER,
    RIGHT_ELBOW,
    RIGHT_HAND
};

Vector3 GetBonePos(DWORD addr, BoneID boneid) {
    Vector3 pos {};
    addr = 0xC + addr + (0x30 * (int)boneid);
    pos.x = RPM<float>(addr);
    pos.y = RPM<float>(addr + 0x10);
    pos.z = RPM<float>(addr + 0x10*2);

    return pos;
}

enum class Weapon
{
    Glock = 0,
    USP,
    P228,
    Deagle,
    Elite,
    FiveSeven,
    M3,
    XM1014,
    Galil,
    AK47,
    Scout,
    SG552,
    AWP,
    G3SG1,
    Famas,
    M4A1,
    Aug,
    SG550,
    Mac10,
    TMP,
    MP5Navy,
    Ump45,
    P90,
    M249,
    Vest,
    VestHelm,
    FlashBang,
    HEGrenade,
    SmokeGrenade,
    Defuser,
    NightVision,
    C4,
    Max
};

uintptr_t GetEntity(uintptr_t entity_list, int index) {
	return RPM<uintptr_t>(entity_list + (index * 0x10));
}

inline MODULEENTRY32 get_module(const char* modName, DWORD proc_id) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, proc_id);
    if (hSnap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32First(hSnap, &modEntry)) {
            do {
                if (!strcmp(modEntry.szModule, modName)) {
                    CloseHandle(hSnap);
                    return modEntry;
                }
            } while (Module32Next(hSnap, &modEntry));
        }
    }
    MODULEENTRY32 module = { (DWORD)-1 };
    return module;
}

void init_modules() {
    Game::client = (uintptr_t)get_module(xorstr_("client.dll"), Game::PID).modBaseAddr;
    Game::engine = (uintptr_t)get_module(xorstr_("engine.dll"), Game::PID).modBaseAddr;
}
