#include <Windows.h>
#include <iostream>
#include "csgo.hpp"

uintptr_t modBase;
uintptr_t engine;
uintptr_t localPlayer;
uintptr_t clientState;

struct Vec3 { float x, y, z; };

Vec3 GetPlayerBonePos(int id) {
    Vec3 pos;
    uintptr_t dwBoneMatrix = *(uintptr_t*)(localPlayer + hazedumper::netvars::m_dwBoneMatrix);
    pos.x = *(float*)(dwBoneMatrix + 0x30 * 8 + 0x0C);
    pos.y = *(float*)(dwBoneMatrix + 0x30 * 8 + 0x1C);
    pos.z = *(float*)(dwBoneMatrix + 0x30 * 8 + 0x2C);
    return pos;
}

Vec3 GetEnemyBonePos(int boneId, int id) {
    Vec3 pos;
    uintptr_t dwEntity = *(uintptr_t*)(modBase + hazedumper::signatures::dwEntityList + (id * 0x10));
    uintptr_t dwBoneMatrix = *(uintptr_t*)(dwEntity + hazedumper::netvars::m_dwBoneMatrix);
    pos.x = *(float*)(dwBoneMatrix + 0x30 * 8 + 0x0C);
    pos.y = *(float*)(dwBoneMatrix + 0x30 * 8 + 0x1C);
    pos.z = *(float*)(dwBoneMatrix + 0x30 * 8 + 0x2C);
    return pos;
}

float GetDistance(Vec3 playerPos, Vec3 enemyPos) {
    float dist = sqrt(pow(playerPos.x - enemyPos.x, 2) + pow(playerPos.y - enemyPos.y, 2) + pow(playerPos.z - enemyPos.z, 2));
    return dist;
}

Vec3 GetAngles(Vec3 source, Vec3 destination) {
    Vec3 rotate;
    float a = destination.x - source.x;
    float b = destination.y - source.y;
    float c = sqrt(a * a + b * b);
    float angles = atan(b / a);
    if (a > 0) {
        rotate.y = angles * (180 / 3.14159265359);
    }
    else {
        rotate.y = angles * (180 / 3.14159265359) + 180;
    }

    b = destination.z - source.z;
    angles = atan(b / c);
    rotate.x = -angles * (180 / 3.14159265359);

    return rotate;
}

int GetClosestEnemyId() {
    float closestDist = FLT_MAX;
    int closestEnemyId;
    for (short int i = 0; i < 64; i++) {
        uintptr_t entity = *(uintptr_t*)(modBase + hazedumper::signatures::dwEntityList + i * 0x10);
        if (entity) {
            Vec3 playerHeadPos = GetPlayerBonePos(8);
            Vec3 enemyBonePos = GetEnemyBonePos(8, i);

            Vec3 rotate = GetAngles(playerHeadPos, enemyBonePos);
            Vec3 angles = *(Vec3*)(clientState + hazedumper::signatures::dwClientState_ViewAngles);
            float diff = sqrt(pow((angles.x + angles.y) - (rotate.x + rotate.y), 2));

            bool dormant = *(bool*)(entity + hazedumper::signatures::m_bDormant);
            int enemyTeamId = *(int*)(entity + hazedumper::netvars::m_iTeamNum);
            int enemyHealth = *(int*)(entity + hazedumper::netvars::m_iHealth);
            int playerTeamId = *(int*)(localPlayer + hazedumper::netvars::m_iTeamNum);
            if (enemyTeamId != playerTeamId && diff < closestDist && enemyHealth > 0 && enemyHealth < 101 && !dormant) {
                closestDist = diff;
                closestEnemyId = i;
            }
        }
    }
    return closestEnemyId;
}

DWORD WINAPI MainThread(HMODULE hModule) {
    AllocConsole();
    FILE* file;
    freopen_s(&file, "CONOUT$", "w", stdout);

    modBase = (uintptr_t)GetModuleHandle(L"client.dll");
    engine = (uintptr_t)GetModuleHandle(L"engine.dll");
    localPlayer = *(uintptr_t*)(modBase + hazedumper::signatures::dwLocalPlayer);
    clientState = *(uintptr_t*)(engine + hazedumper::signatures::dwClientState);

    while (true)
    {
        if (GetAsyncKeyState(VK_XBUTTON2))
        {
            int closestEnemy = GetClosestEnemyId();
            //std::cout << closestEnemy << std::endl;
            if (closestEnemy > 0 && closestEnemy < 64)
            {
                Vec3 playerHeadPos = GetPlayerBonePos(8);
                Vec3 enemyBonePos = GetEnemyBonePos(8, closestEnemy);

                Vec3 rotate = GetAngles(playerHeadPos, enemyBonePos);
                //std::cout << rotate.x << " " << rotate.y << std::endl;
                *(Vec3*)(clientState + hazedumper::signatures::dwClientState_ViewAngles) = rotate;
            }
        }

        if (GetAsyncKeyState(VK_END) & 1)
        {
            if (file) { fclose(file); }
            FreeConsole();
            FreeLibraryAndExitThread(hModule, 0);
            return 0;
        }
    }
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, 0);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}