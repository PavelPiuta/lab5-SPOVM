#include "pch.h"

using namespace std;
namespace fs = std::filesystem;						

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call,LPVOID lpReserved) 
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		cout << "Ñonnect process!" << endl;
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

struct threadMessage														 
{
	threadMessage(HANDLE* eventHandle_, HANDLE* ioHandle_)
	{
		eventHandle = eventHandle_;
		ioHandle = ioHandle_;
	}
	HANDLE* eventHandle;
	HANDLE* ioHandle;
};

extern "C" __declspec(dllexport) DWORD WINAPI ReaderThread(void* par)					
{
	threadMessage message = *(threadMessage*)par;
	fs::path filesPath(fs::current_path() /= fs::path("files"));						
	fs::create_directories(filesPath);
	HANDLE eventHandle = CreateEvent(NULL, FALSE, FALSE, "Event");						
	if (eventHandle == NULL)
		cout << "Error creating event" << endl;

	for (auto it : fs::directory_iterator(filesPath))									
	{
		cout << "Reading " << it.path().filename() << endl;								
		char inputBuffer[BUFSIZ + 1];													
		string readedStr = "";
		int readedBytesCount = 0;														
		
		HANDLE fileHandle = CreateFileW(it.path().c_str(),								
			GENERIC_READ,																
			NULL,																		
			NULL,																		
			OPEN_EXISTING,																
			FILE_FLAG_OVERLAPPED,														
			NULL);																		
		if (fileHandle == INVALID_HANDLE_VALUE)											
			cout << "Error opening file" << endl;

		OVERLAPPED params;																
		params.hEvent = eventHandle;													
		params.Offset = 0;																
		params.OffsetHigh = 0;															

		do
		{
			ReadFile(fileHandle, inputBuffer, BUFSIZ, (LPDWORD)&readedBytesCount, &params); 
			WaitForSingleObject(eventHandle, INFINITE);
			inputBuffer[params.InternalHigh] = 0;										
			readedStr += inputBuffer;
			params.Offset += params.InternalHigh;										
		} while (params.InternalHigh == BUFSIZ);										
		CloseHandle(fileHandle);

		if (readedStr.size())															
		{
			WriteFile(*message.ioHandle, readedStr.c_str(), readedStr.size(), NULL, NULL);
			Sleep(100);
			PulseEvent(*message.eventHandle);
			WaitForSingleObject(*message.eventHandle, INFINITE);
		}
	}
	CloseHandle(eventHandle);
	return 0;
}

extern "C" __declspec(dllexport) DWORD WINAPI WriterThread(void* par)					
{
	threadMessage message = *(threadMessage*)par;
	HANDLE fileHandle = CreateFileA("output.txt", GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, NULL); 
	HANDLE eventHandle = CreateEvent(NULL, FALSE, FALSE, "Event");

	OVERLAPPED params;																
	params.hEvent = eventHandle;													
	params.Offset = 0;																
	params.OffsetHigh = 0;															

	while (true)
	{
		WaitForSingleObject(*message.eventHandle, INFINITE);
		string readedString;
		while (true)
		{
			char buffer[BUFSIZ + 1];
			int bytesReaded;
			ReadFile(*message.ioHandle, buffer, BUFSIZ, (LPDWORD)&bytesReaded, NULL); 
			buffer[bytesReaded] = 0;
			readedString += buffer;
			if (string(buffer).size() != BUFSIZ)
				break;

		}

		WriteFile(fileHandle, readedString.c_str(), readedString.size(), NULL, &params);
		params.Offset += readedString.size();
		WaitForSingleObject(eventHandle, INFINITE);
		Sleep(100);
		PulseEvent(*message.eventHandle);
	}
	CloseHandle(fileHandle);
	return 0;
}