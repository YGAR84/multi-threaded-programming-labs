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
		write(0, text, strlen(text));
		fflush(0);
	}

}

void * threadBody(void * param) 
{
	printStrings((char*)param);
}

int main()
{
	pthread_t thread;
	
	char text[10];
	int code;
	
	sprintf(text, "a");

	
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
	
	return 0;
}