#include <iostream>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <string>
#include <dlfcn.h>
using namespace std;

pthread_t ReaderThread_t;                          
pthread_t WriterThread_t;

typedef void*(*FUNC)(void*);
FUNC WriterThread;
FUNC ReaderThread;

void f(int p)                                       
{
	if (p==10)
		pthread_kill(ReaderThread_t,p);             
	if (p==12 || p==13)
		pthread_kill(WriterThread_t,p);
};

int main()
{
	void* sohandle = dlopen("/home/pavel/Desktop/lb5/IODLL.so", RTLD_LAZY | RTLD_LOCAL);  
	if (!sohandle)
	{
		cout << "Error opening dynamic lib!\n";
		exit(0);
	}

	WriterThread = (FUNC)dlsym(sohandle,"_Z12WriterThreadPv");            
	if (!WriterThread)
	{
		cout << "Error opening WriterThread function!\n";
		exit(0);
	}
	if (!(ReaderThread = (FUNC)dlsym(sohandle,"_Z12ReaderThreadPv")))      
	{
		cout << "Error opening ReaderThread function!\n";
		exit(0);
	}

	int pipes[2];                                                          
	pipe(pipes);

	signal(10,f);                                                          
	signal(12,f);
	signal(13,f);

	pthread_create(&ReaderThread_t, NULL, ReaderThread, &pipes[1]);        
	pthread_create(&WriterThread_t, NULL, WriterThread, &pipes[0]);       

	getchar();
	dlclose(sohandle);                                                     
	return 0;
}
