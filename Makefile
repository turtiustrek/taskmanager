CC = g++
all: dll injector
dll:src/dll/dllmain.cpp
	$(CC) -shared -fPIC  src/dll/dllmain.cpp src/dll/libs/libmem/libmem/libmem.c -o dllmain.dll -lPsapi  -lgdi32 -Wall 

injector: src/injector/main.cpp
	$(CC) .\src\injector\main.cpp -lshlwapi -lz -static -o injector.exe