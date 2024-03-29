#include <sys/types.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void printTenStrings(const char * text) 
{
	int i;
	for (i = 0; i < 10; ++i)
	{
		printf("%d: %s\n", i + 1, text);
	}
}

void * threadBody(void * param) 
{
	printTenStrings("I'm thread");
}

int main()
{
	pthread_t thread;

	int code;
	
	code = pthread_create(&thread, NULL, threadBody, NULL);
	if (code != 0)
	{
		perror("Thread create error");
		exit(0);
	}
	
	code = pthread_join(thread, NULL);
	if (code != 0)
	{
		perror("Joining thread error");
	}
	printTenStrings("yes");
	pthread_exit(NULL);
}