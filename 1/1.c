#include <sys/types.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

//this function prints 10 strings
void printTenStrings(const char * text) 
{
	int i;
	for (i = 0; i < 10; ++i)
	{
		printf("%d: %s\n", i + 1, text);
	}
}

//start routine function for thread
void * threadBody(void * param) 
{
	printTenStrings("I'm thread");
}

int main()
{
	pthread_t thread;

	//pthread_create create new thread with params : 
	//	pointer on struct pthread_t
	//	pointer on struct pthread_attr_t 
	//				(if we want to create thread with default attrs, use NULL)
	//	pointer on start rountine function
	//	args for function threadBody(if no args use NULL)
	int code = pthread_create(&thread, NULL, threadBody, NULL);
	
	// We have an error if code != 0
	if (code != 0)
	{
		//print error in std::err with function perror
		perror("Thread create error");
		exit(0);
	}
	printTenStrings("yes");
	
	//Use pthread_exit to terminate current thread
	pthread_exit(NULL);
}