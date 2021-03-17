/*
Written by: turtius
Description: DLL for task manager which draws bitmaps
Repo: https://github.com/turtiustrek/taskmanager
*/

#include <windows.h>
#include "libs/libmem/libmem/libmem.hpp"
#include "pattern.hpp"
#include <stdio.h>
#include <iostream>
#include <Windows.h>
#include <conio.h>
#include <stdint.h>
#include <wingdi.h>

//you can change these
#define FAKE_CORES 1024 //this looks nice ngl. Cant be less then or equal to  0x40
int blockWidth = 50;
//#define SHOW_BITMAP_SCAN_MSG //shows what files are being loaded into memory. If you have alot of bitmaps keep this commented.

mem_voidptr_t UpdateData = (mem_voidptr_t)MEM_BAD;
mem_voidptr_t GetBlockWidth = (mem_voidptr_t)MEM_BAD;
mem_voidptr_t IsServer = (mem_voidptr_t)MEM_BAD;
void *handler = (mem_voidptr_t)MEM_BAD;
void *GlobalSettings = (mem_voidptr_t)MEM_BAD;

int __fastcall (*GetBlockColors)(void *, int core, long *background, long *border);
int __fastcall (*SetBlockData)(void *, int, const wchar_t *string, long background, long border);
//Position inside the GLOBAL_SETTINGS_TASKMGR
#define GLOBAL_SETTINGS_CPU_OFFSET 0x944 //not relative to BaseAdress but GLOBAL_SETTINGS_TASKMGR
//task manager handle
mem_module_t mod = {0};
mem_tstring_t process_path = (mem_tstring_t)NULL;

//task manager block width

//bitmaps
WIN32_FIND_DATAW data;
wchar_t dllDir[MAX_PATH];
wchar_t bitmapDir[MAX_PATH];
wchar_t frame[] = L"\\..\\frames\\*.bmp";
int frames = 0;
int currentFrame = 0;
int commandFrame = 0;
char *bitmapPixels = (char *)MEM_BAD;

int64_t __fastcall UpdateDataHook(void *ret)
{

    handler = ret;
    switch (commandFrame)
    {
    case 0:
        currentFrame++;
        if (currentFrame >= frames)
        {
            currentFrame = 0;
        }
        break;
    case 1:
        break;
    default:
        break;
    }
    long v10;
    long v11;
    wchar_t w[5];
    for (int i = 0; i < FAKE_CORES; i++)
    {
        int pixel = *(bitmapPixels + (i + (currentFrame * 1024)));
        swprintf_s(w, L"%d%%", pixel);
        GetBlockColors(ret, pixel, &v11, &v10);
        SetBlockData(ret, i, w, v11, v10);
    }

    return 1;
}
//Used to alter the size of the block
int64_t __fastcall GetBlockWidthHook(void *ret)
{

    return blockWidth;
}
//Used to get GlobalSettings
//TODO: not do this?
bool __fastcall IsServerHook(void *ret)
{
    GlobalSettings = ret;
    return false;
}
//Console printing
void printDone(HANDLE console)
{
    SetConsoleTextAttribute(console, 10);
    std::cout << "Done" << std::endl;
    SetConsoleTextAttribute(console, 7);
}
void printFail(HANDLE console)
{
    SetConsoleTextAttribute(console, 12);
    std::cout << "Fail" << std::endl;
    SetConsoleTextAttribute(console, 7);
}
void printnullptr(HANDLE console, void *ptr)
{
    if (ptr == (mem_voidptr_t)MEM_BAD)
    {
        printFail(console);
    }
    else
    {
        printDone(console);
    }
}
int map(int x, int in_min, int in_max, int out_min, int out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
//main thread of dllmain
DWORD WINAPI attach(LPVOID dllHandle)

{
    AllocConsole();

    FILE *fDummy;
    //Re-allocate console
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    //process(taskmgr) path
    mem_in_get_process_path(&process_path);
    mod = mem_in_get_module(process_path);

    std::cout << "Base address " << (void *)mod.base << std::endl;
    std::cout << "DLL address " << (void *)attach << std::endl;
    DWORD verHandle = 0;
    UINT size = 0;
    LPBYTE lpBuffer = NULL;
    DWORD verSize = GetFileVersionInfoSize(process_path, &verHandle);

    if (verSize != NULL)
    {
        LPSTR verData = new char[verSize];

        if (GetFileVersionInfo(process_path, verHandle, verSize, verData))
        {
            if (VerQueryValueW(verData, L"\\", (VOID FAR * FAR *)&lpBuffer, &size))
            {
                if (size)
                {
                    VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *)lpBuffer;
                    if (verInfo->dwSignature == 0xfeef04bd)
                    {
                        SetConsoleTextAttribute(hConsole, 14);
                        std::cout << "Process Version: " << ((verInfo->dwFileVersionMS >> 16) & 0xffff) << '.' << ((verInfo->dwFileVersionMS >> 0) & 0xffff) << '.' << ((verInfo->dwFileVersionLS >> 16) & 0xffff) << '.' << ((verInfo->dwFileVersionLS >> 0) & 0xffff) << std::endl;
                        SetConsoleTextAttribute(hConsole, 7);
                    }
                }
            }
        }
        delete[] verData;
    }
    //Choose the table that is going to be used to find the functions
    for (int i = 0; i < (int)(sizeof(table) / sizeof(table[0])); i++)
    {
        std::cout << "Table task manager version:";
        SetConsoleTextAttribute(hConsole, 11);
        std::cout << table[i].version << std::endl;
        SetConsoleTextAttribute(hConsole, 7);
        std::cout << "Finding UpdateData function...";
        UpdateData = mem::in::scan(table[i].UpdateDataPattern, PATTERN_BYTES, mod.base, mod.end);
        printnullptr(hConsole, UpdateData);
        std::cout << "Finding GetBlockWidth function...";
        GetBlockWidth = mem::in::scan(table[i].GetBlockWidthPattern, PATTERN_BYTES, mod.base, mod.end);
        printnullptr(hConsole, GetBlockWidth);
        std::cout << "Finding IsServer function...";
        IsServer = mem::in::scan(table[i].IsServerPattern, PATTERN_BYTES, mod.base, mod.end);
        printnullptr(hConsole, IsServer);
        std::cout << "Finding GetBlockColors function...";
        GetBlockColors = (decltype(GetBlockColors))(mem::in::scan(table[i].GetBlockColorsPattern, PATTERN_BYTES, mod.base, mod.end));
        printnullptr(hConsole, (void *)GetBlockColors);
        std::cout << "Finding SetBlockData function...";
        SetBlockData = (decltype(SetBlockData))(mem::in::scan(table[i].SetBlockDataPattern, PATTERN_BYTES, mod.base, mod.end));
        printnullptr(hConsole, (void *)SetBlockData);
        if (UpdateData == (mem_voidptr_t)MEM_BAD || GetBlockWidth == (mem_voidptr_t)MEM_BAD || IsServer == (mem_voidptr_t)MEM_BAD || GetBlockColors == (mem_voidptr_t)MEM_BAD || SetBlockData == (mem_voidptr_t)MEM_BAD)
        {
            //break if all tables have been checked
            if (i == (sizeof(table) / sizeof(table[0])) - 1)
            {
                break;
            }
            else
            {
                SetConsoleTextAttribute(hConsole, 12);
                std::cout << "One or more functions were not found, attempting alternative table" << std::endl;
                SetConsoleTextAttribute(hConsole, 7);
            }
        }
        else
        {
            break;
        }
    }
    if (UpdateData == (mem_voidptr_t)MEM_BAD || GetBlockWidth == (mem_voidptr_t)MEM_BAD || IsServer == (mem_voidptr_t)MEM_BAD || GetBlockColors == (mem_voidptr_t)MEM_BAD || SetBlockData == (mem_voidptr_t)MEM_BAD)
    {
        SetConsoleTextAttribute(hConsole, 12);
        std::cout << "One or more functions were not found, waiting for exit" << std::endl;
        SetConsoleTextAttribute(hConsole, 7);
        return 0;
    }
    std::cout << "Placing hooks...";
    //Gateway is NOT used! IsServer might be problamatic
    mem::in::detour_trampoline(UpdateData, (void *)UpdateDataHook, mem::in::detour_size(MEM_ASM_x86_JMP64), MEM_ASM_x86_JMP64);
    mem::in::detour_trampoline(IsServer, (void *)IsServerHook, mem::in::detour_size(MEM_ASM_x86_JMP64), MEM_ASM_x86_JMP64);
    mem::in::detour_trampoline(GetBlockWidth, (void *)GetBlockWidthHook, mem::in::detour_size(MEM_ASM_x86_JMP64), MEM_ASM_x86_JMP64);
    printDone(hConsole);
    std::cout << "Waiting for GlobalSettings to populate...";
    while (GlobalSettings == (mem_voidptr_t)MEM_BAD)
    {
    };
    printDone(hConsole);
    std::cout << "Altering CPU count...";
    //Cast it as a unsigned short since the CPU 'settings' expects two bytes
    unsigned short *cpu_count = (unsigned short *)((char *)GlobalSettings + GLOBAL_SETTINGS_CPU_OFFSET);
    *cpu_count = FAKE_CORES;
    printDone(hConsole);
    std::cout << "Scanning bitmaps and loading in memory..." << std::endl;
    GetModuleFileNameW((HMODULE)dllHandle, dllDir, sizeof(dllDir));
    memcpy(bitmapDir, dllDir, sizeof(dllDir));
    wcscat(bitmapDir, frame);
    std::wcout << "Bitmap scan at: " << bitmapDir << std::endl;
    HANDLE hFind = FindFirstFileW(bitmapDir, &data);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        wchar_t files[MAX_PATH];
        HDC hdc;
        COLORREF col;
        HBITMAP oldbitmap;
        BITMAP bm = {0};
        int allocate = 0;
        int average = 0;
        int byte = 0;
        do
        {
            //TODO: Fix this bodge
            swprintf_s(files, L"%s\\..\\frames\\%s", dllDir, data.cFileName);
            HBITMAP hBitMap = (HBITMAP)::LoadImageW(NULL, files, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
            GetObject(hBitMap, sizeof(bm), &bm);
            hdc = CreateCompatibleDC(NULL);
            oldbitmap = (HBITMAP)SelectObject(hdc, hBitMap);
#ifdef SHOW_BITMAP_SCAN_MSG
            SetConsoleTextAttribute(hConsole, 11);
            std::wcout << L"Loading: " << data.cFileName << std::endl;
            SetConsoleTextAttribute(hConsole, 7);
            std::cout << " Size W:" << bm.bmWidth << " L:" << bm.bmHeight << std::endl;
#endif

            allocate += bm.bmWidth * bm.bmHeight;

            //realloc if it has not been malloc'd already
            if (bitmapPixels == (char *)MEM_BAD)
            {
#ifdef SHOW_BITMAP_SCAN_MSG
                std::cout << " Allocating " << allocate << " bytes" << std::endl;
#endif
                bitmapPixels = (char *)malloc(allocate);
            }
            else
            {
#ifdef SHOW_BITMAP_SCAN_MSG
                std::cout << " Reallocating " << allocate << " bytes" << std::endl;
#endif
                bitmapPixels = (char *)realloc(bitmapPixels, allocate);
            }

            for (int y = 0; y < bm.bmHeight; y++)
            {
                for (int x = 0; x < bm.bmWidth; x++)
                {
                    //TODO: avoid this
                    col = GetPixel(hdc, x, y);
                    average = ((GetRValue(col) + GetGValue(col) + GetBValue(col)) / 3);
                    *(bitmapPixels + byte) = (char)map(average, 255, 0, 0, 100);
                    byte++;
                }
            }
            // Clean up
            SelectObject(hdc, oldbitmap);
            DeleteDC(hdc);
            frames++;
            //std::cout << bitmapmem<< std::endl;
            //std::cout << data.cFileName << std::endl;
        } while (FindNextFileW(hFind, &data));
        FindClose(hFind);
    }
    if (frames > 0)
    {
        SetConsoleTextAttribute(hConsole, 11);
        std::cout << "Found frames: " << frames << std::endl;
        SetConsoleTextAttribute(hConsole, 7);
    }
    else
    {
        SetConsoleTextAttribute(hConsole, 12);
        std::cout << "No frames found, waiting for exit" << std::endl;
        return 0;
    }

    std::cout << "Waiting for handler to populate{Switch over to the performance tab}...";
    while (handler == (mem_voidptr_t)MEM_BAD)
    {
    };
    printDone(hConsole);
    SetConsoleTextAttribute(hConsole, 10);
    std::cout << "Loaded sucessfully" << std::endl;

    SetConsoleTextAttribute(hConsole, 6);
    std::cout << "Press 'h' to see help" << std::endl;
    SetConsoleTextAttribute(hConsole, 7);
    while (true)
    {

        switch (getch())
        {
        case ('H'):
        case ('h'):
            std::cout << "--------------------------" << std::endl;
            std::cout << "Key - Description" << std::endl;
            std::cout << "H - View this help screen" << std::endl;
            std::cout << "I- View current values" << std::endl;
            std::cout << "D - Increment BlockWidth" << std::endl;
            std::cout << "A - Decrement BlockWidth" << std::endl;
            std::cout << "O - Play frame" << std::endl;
            std::cout << "P - Pause frame" << std::endl;
            std::cout << "W - Increment frame" << std::endl;
            std::cout << "S-  Decrement frame" << std::endl;
            std::cout << "--------------------------" << std::endl;
            break;
        case ('D'):
        case ('d'):
            blockWidth++;
            if (blockWidth <= 1)
            {
                blockWidth = 1;
            }
            std::cout << "--------------------------" << std::endl;
            std::cout << "Current Block Width:" << blockWidth << std::endl;
            std::cout << "--------------------------" << std::endl;
            break;
        case ('A'):
        case ('a'):
            blockWidth--;
            if (blockWidth <= 1)
            {
                blockWidth = 1;
            }
            std::cout << "--------------------------" << std::endl;
            std::cout << "Current Block Width:" << blockWidth << std::endl;
            std::cout << "--------------------------" << std::endl;
            break;
        case ('I'):
        case ('i'):
            std::cout << "--------------------------" << std::endl;
            std::cout << "Current Block Width:" << blockWidth << std::endl;
            std::cout << "Current frame:" << currentFrame << std::endl;
            std::cout << "--------------------------" << std::endl;
            break;
        case ('O'):
        case ('o'):
            commandFrame = 0;
            std::cout << "--------------------------" << std::endl;
            std::cout << "Playing Frames" << std::endl;
            std::cout << "--------------------------" << std::endl;
            break;
        case ('P'):
        case ('p'):
            commandFrame = 1;
            std::cout << "--------------------------" << std::endl;
            std::cout << "Pausing Frames" << std::endl;
            std::cout << "--------------------------" << std::endl;
            break;

        case ('W'):
        case ('w'):
            currentFrame++;
            if (currentFrame >= frames)
            {
                currentFrame = frames - 1;
            }
            std::cout << "--------------------------" << std::endl;
            std::cout << "Current frame:" << currentFrame << std::endl;
            std::cout << "--------------------------" << std::endl;
            break;

        case ('S'):
        case ('s'):
            currentFrame--;
            if (currentFrame <= 0)
            {
                currentFrame = 0;
            }
            std::cout << "--------------------------" << std::endl;
            std::cout << "Current frame:" << currentFrame << std::endl;
            std::cout << "--------------------------" << std::endl;
            break;
        default:
            std::cout << "Invalid keypress.Press 'h' to see help" << std::endl;
            break;
        }
    }
    return true;
}
extern "C" BOOL APIENTRY DllMain(HMODULE hModule,
                                 DWORD ul_reason_for_call,
                                 LPVOID lpReserved)

{
    switch (ul_reason_for_call)
    {
        DWORD dwThreadId;
    case DLL_PROCESS_ATTACH:
        CreateThread(NULL, 0, attach, hModule, 0, &dwThreadId);
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        if (bitmapPixels != (char *)MEM_BAD)
        {
            free(bitmapPixels);
            MessageBoxW(NULL, L"DLL exited successfully", L"Info", MB_ICONINFORMATION);
        }

        break;
    }
    return TRUE;
}