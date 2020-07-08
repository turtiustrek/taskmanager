// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <iostream>
#include <Windows.h>
#include <stdint.h>
#include <Psapi.h>
#include "video.h"
#include "detours.h"
#include <thread>   
#define MEM(x) (*(int *)(x))



// base address
static uintptr_t BaseAddress;
HANDLE process;
//all of these values must be relative to BaseAddress
#define GLOBAL_SETTINGS_TASKMGR  0xfb550 
#define GLOBAL_SETTINGS_CPU_OFFSET 0x944
#define FAKE_CORES 1024//this looks nice ngl. Cant be less then or equal to  0x40

#define UPDATE_DATA_FUNCTION 0xab738 // CpuHeatMap::UpdateData
#define GET_BLOCK_COLOURS_FUNCTION 0xaacbc//CpuHeatMap::GetBlockColors
#define SET_BLOCK_DATA_FUNCTION 0xab614//CpuHeatMap::SetBlockData

int( *UpdateData)(void *);
void(*GetBlockColours)(void *, int core, long *background, long *border);
void(*SetBlockData)(void *, int, const wchar_t* string, long background, long border);

int frame =0 ;
int *globalHandle;
int ran =0;
//Runs every update in task manager.
int  MyUpdateData(int *handle)
{
	if (frame > 4) {
		frame = 0;
	}
	long v10;
	long v11;
	wchar_t w[5];
	for (int i = 0; i < FAKE_CORES; i++) {
		int pixel = test[i + (frame*1024)];
		swprintf_s(w, L"%d%%",pixel);


		GetBlockColours(handle, pixel, &v11, &v10);
		SetBlockData(handle, i, w, v11, v10);

	}
	frame += 1;
		
	GetCurrentThreadId();

	
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
	//If it made it this far then load the file

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
