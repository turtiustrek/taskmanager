// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <iostream>
#include <Windows.h>
#include <stdint.h>
#include <Psapi.h>

#include "detours.h"
#define MEM(x) (*(int *)(x))



// base address
static uintptr_t BaseAddress;
HANDLE process;
//all of these values must be relative to BaseAddress
#define GLOBAL_SETTINGS_TASKMGR  0xfb550 
#define GLOBAL_SETTINGS_CPU_OFFSET 0x944
#define FAKE_CORES 567//this looks nice ngl. Cant be less then or equal to  0x40

#define UPDATE_DATA_FUNCTION 0xab738 // CpuHeatMap::UpdateData
#define GET_BLOCK_COLOURS_FUNCTION 0xaacbc//CpuHeatMap::GetBlockColors
#define SET_BLOCK_DATA_FUNCTION 0xab614//CpuHeatMap::SetBlockData

int( *UpdateData)(void *);
void(*GetBlockColours)(void *, int, int *background, int *border);
void(*SetBlockData)(void *, int, const wchar_t* string, int background,int border);

long map(long x, long in_min, long in_max, long out_min, long out_max) {
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
//Runs every update in task manager.
int  MyUpdateData(int *handle)
{
	int v10;
	int v11;
	wchar_t w[5];
	for (int i = 0; i < FAKE_CORES; i++) {
		swprintf_s(w, L"%d",i);
		GetBlockColours(handle, map(100, 0, 220, 0, 100) , &v11, &v10);
		SetBlockData(handle, i, w,v11,v10);
		GetCurrentThreadId();
	}

	return 1;
}
bool attach() {
	AllocConsole();
	FILE *fDummy;
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	printf("Base address 0x%I64X\n", BaseAddress);
	//Functions
	UpdateData = (decltype(UpdateData))(BaseAddress + UPDATE_DATA_FUNCTION);
	GetBlockColours = (decltype(GetBlockColours))(BaseAddress + GET_BLOCK_COLOURS_FUNCTION);
	SetBlockData = (decltype(SetBlockData))(BaseAddress + SET_BLOCK_DATA_FUNCTION);


	printf("Applying fake cores...\n");
	MEM(BaseAddress + GLOBAL_SETTINGS_TASKMGR + GLOBAL_SETTINGS_CPU_OFFSET) = FAKE_CORES;
    //Detour the UpdateData function
	DetourTransactionBegin();
	printf("Detouring function from 0x%p to 0x%p\n", &UpdateData, &MyUpdateData);
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)UpdateData, MyUpdateData);
	DetourTransactionCommit();

	return true;
}
// find base ptr dynamically
//https://github.com/byteandahalf/mc10_testmod
DWORD_PTR GetProcessBaseAddress(DWORD processID)
{
	DWORD_PTR baseAddress = 0;
	HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
	HMODULE* moduleArray;
	LPBYTE moduleArrayBytes;
	DWORD bytesRequired;

	if (processHandle)
	{
		if (EnumProcessModules(processHandle, NULL, 0, &bytesRequired))
		{
			if (bytesRequired)
			{
				moduleArrayBytes = (LPBYTE)LocalAlloc(LPTR, bytesRequired);

				if (moduleArrayBytes)
				{
					unsigned int moduleCount;

					moduleCount = bytesRequired / sizeof(HMODULE);
					moduleArray = (HMODULE*)moduleArrayBytes;

					if (EnumProcessModules(processHandle, moduleArray, bytesRequired, &bytesRequired))
					{
						baseAddress = (DWORD_PTR)moduleArray[0];
					}

					LocalFree(moduleArrayBytes);
				}
			}
		}

		CloseHandle(processHandle);
	}

	return baseAddress;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )

{  

	DWORD procId = GetCurrentProcessId();
	process = OpenProcess(PROCESS_ALL_ACCESS | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, FALSE, procId);
	BaseAddress = (uintptr_t)GetProcessBaseAddress(procId);
		

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
	
		return attach();
    case DLL_THREAD_ATTACH:
	
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;

}
