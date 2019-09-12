#include <sys/types.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define numOfThreads 4
#define numOfStrings 4
#define stringSize 20

void printStrings(char ** texts) 
{
	int i;
	for(i = 0; i < numOfStrings; ++i)
	{
		printf("%d:%s\n", i + 1, texts[i]);
	}
}

void * threadBody(void * param) 
{
	printStrings((char**)param);
}

int main()
{
	pthread_t threads[numOfThreads];
	char **texts = (char**)malloc(sizeof(char*) * numOfThreads * numOfStrings);
	int code;
	
	int i;
	for (i = 0; i < numOfThreads; ++i)
	{
		int j;
		for (j = 0; j < numOfStrings; ++j)
		{
			texts[i * numOfStrings + j] = (char*)malloc(sizeof(char) * stringSize);
			sprintf(texts[i * numOfStrings + j], "Thread %d %d", i, i * numOfStrings+ j);
		}
	}
	
	for (i = 0; i < numOfThreads; ++i)
	{
		code = pthread_create(&threads[i], NULL, threadBody, (void*)(&texts[i * numOfStrings]));
		if (code != 0)
		{
			perror("Thread create error");
			exit(0);
		}
	}

	for (i = 0; i < numOfThreads; ++i)
	{
		code = pthread_join(threads[i], NULL);
		if (code != 0)
		{
			perror("Joing thread error:");
		}
	}
	
	for (i = 0; i < numOfThreads * numOfStrings; ++i)
	{
		free(texts[i]);
	}
	free(texts);
	return 0;
}