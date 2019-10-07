#include <sys/types.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <strings.h>

void printStrings(const char * text) 
{
	while(1)
	{
		write(STDOUT_FILENO, text, strlen(text));
		fflush(STDOUT_FILENO);
	}

}

void * threadBody(void * param) 
{
	printStrings((char*)param);
}

int main()
{
	pthread_t thread;
	
	char text[] = "a";
	int code;
	
	code = pthread_create(&thread, NULL, threadBody, (void*)text);
	if (code != 0)
	{
		perror("Thread create error");
		exit(0);
	}
	
	sleep(2);
	
	code = pthread_cancel(thread);
	if (code != 0)
	{
		perror("Canceling thread error");
	}
	
	code = pthread_join(thread, NULL);
	if(code != 0)
	{
		perror("Joining thread error");
	}
	
	return 0;
}