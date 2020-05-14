#include <iostream>
#include <experimental/filesystem>
#include <aio.h>
#include <pthread.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <string>
using namespace std;
namespace fs = std::experimental::filesystem;                       


void* WriterThread(void* par)                                       
{
    int lastSignal;
	int readPipe = *((int*)par);
	int file = open("output.txt",O_WRONLY | O_CREAT, S_IRWXU);      

	sigevent event;                                                
    event.sigev_notify = SIGEV_SIGNAL;                             
    event.sigev_signo = 12;

	sigset_t sigset1;                                               
	sigemptyset(&sigset1);                                         
	sigaddset(&sigset1, 12);                                        
	sigaddset(&sigset1, 13);

    aiocb config;                                                   
    config.aio_fildes = file;                                       
    config.aio_offset = 0;                                          
    config.aio_reqprio = 0;                                        
    config.aio_sigevent = event;                                  

	while(true)
	{
		char buf[BUFSIZ] = {};
		for (int i=0;i<BUFSIZ;i++)
			buf[i] = 0;
        
		sigwait(&sigset1, &lastSignal);                             
		if (lastSignal == 13)
			break;
        
		read(readPipe,buf,BUFSIZ);                                  
		config.aio_buf = buf;                                      
        config.aio_fildes = file;
        config.aio_nbytes = string(buf).size();                     
       
        aio_write(&config);                                         
        sigwait(&sigset1,&lastSignal);
        config.aio_offset += string(buf).size();                    
        raise(10);                                                  
	}	
	close(readPipe);
	close(file);
	return nullptr;
}

void* ReaderThread(void *par)                                      
{
    int lastSignal;
	fs::path filesPath(fs::current_path() /= fs::path("files"));   
	fs::create_directories(filesPath);                             
	int pipe1 = *((int*)par);

    sigevent event;                                                
    event.sigev_notify = SIGEV_SIGNAL;                             
    event.sigev_signo = 10;

	sigset_t sigset1;                                               
	sigemptyset(&sigset1);                                          
	sigaddset(&sigset1, 10);                                        


	aiocb config;                                                  
    config.aio_offset = 0;                                          
    config.aio_nbytes = BUFSIZ;                                     
    config.aio_reqprio = 0;
    config.aio_sigevent = event;                                  

    for (auto it : fs::directory_iterator(filesPath))               
    {
        cout << "Reading " << it.path().filename() << endl;
        int file = open(it.path().c_str(),O_RDONLY);                
        char buf[BUFSIZ] = {};

		config.aio_buf = buf;                                       
        config.aio_fildes = file;

        aio_read(&config);                                         
        sigwait(&sigset1,&lastSignal);
        write(pipe1, buf, string(buf).size());                     
        raise(12);                                                  
        sigwait(&sigset1,&lastSignal);
        close(file);
    }
    close(pipe1);
    raise(13);                                                      
	return nullptr;
}
