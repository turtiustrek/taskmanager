CC = g++
all: dll injector
dll:src/dll/dllmain.cpp
	$(CC) -shared -fPIC  -Isrc/dll/libs/rapidjson/include src/dll/dllmain.cpp src/dll/libs/libmem/libmem/libmem.c -o dllmain.dll -lPsapi  -lgdi32 -lVersion -lShlwapi -Wall 

injector: src/injector/main.cpp
	$(CC) .\src\injector\main.cpp -lshlwapi -lz -static -o injector.exe