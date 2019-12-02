#include <sys/types.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <strings.h>

#define WRITE_ERROR -1

void printStrings(const char * text) 
{
	int num;
	while(1)
	{
		num = write(STDOUT_FILENO, text, strlen(text));
		if(num == WRITE_ERROR)
		{
			perror("Write error");
			pthread_exit(NULL);
		}
		fflush(stdout);
	}
}

void handler(void * arg)
{
	printf("\nI'm canceled\n");
}


void * threadBody(void * param) 
{
	pthread_cleanup_push(handler, NULL);
	printStrings((char*)param);
	pthread_cleanup_pop(0);
	
	pthread_exit(NULL);
}

int main()
{
	pthread_t thread;
	
	char text[] = "a";

	errno = pthread_create(&thread, NULL, threadBody, (void*)text);
	if (errno != 0)
	{
		perror("Thread create error");
		exit(0);
	}
	
	sleep(2);
	
	errno = pthread_cancel(thread);
	if (errno != 0)
	{
		perror("Canceling thread error");
	}
	
	errno = pthread_join(thread, NULL);
	if(errno != 0)
	{
		perror("Joining thread error\n");
	}
	
	pthread_exit(NULL);
}
