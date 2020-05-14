#include <iostream>
#include <Windows.h>
#include <filesystem>
using namespace std;
namespace fs = std::filesystem;

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

typedef DWORD(WINAPI* importFunc)(void* par);
importFunc ReaderThread;
importFunc WriterThread;

int main()
{
	HINSTANCE dllHandler = LoadLibrary("ReadWriteDLL");													
	if (!dllHandler)
		cout << "END OF PROGRAMM!\n";

	if (((ReaderThread = (importFunc)GetProcAddress(dllHandler, "_ReaderThread@4")) == NULL) ||			
		(WriterThread = (importFunc)GetProcAddress(dllHandler, "_WriterThread@4")) == NULL)
		cout << "ERROR IMPORTING " << GetLastError() << "\n";

	HANDLE readPipeHandle, writePipeHandle, syncEventHandle = CreateEventA(NULL, TRUE, FALSE, NULL);    

	if (CreatePipe(&readPipeHandle, &writePipeHandle, NULL, 32 * BUFSIZ) == 0)							
																										
		cout << "Error creating pipe" << endl;

	HANDLE WriterThreadHandle = CreateThread(NULL, NULL, WriterThread, new threadMessage(&syncEventHandle, &readPipeHandle), NULL, NULL);
	
	HANDLE ReaderThreadHandle = CreateThread(NULL, NULL, ReaderThread, new threadMessage(&syncEventHandle, &writePipeHandle), NULL, NULL);
	
	WaitForSingleObject(ReaderThreadHandle, INFINITE);				
	Sleep(300);

	FreeLibrary(dllHandler);										
	getchar();
	return 0;
}