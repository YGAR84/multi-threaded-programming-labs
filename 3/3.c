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

char** createStrings(int threads, int strnum)
{
	char** result = (char**)malloc(sizeof(char*) * threads * strnum);
	if(result == NULL)
	{
		printf("Error while allocating memory\n");
		exit(0);
	}
	
	int i;
	for(i = 0; i < threads; ++i)
	{
		int j;
		for(j = 0; j < strnum; ++j)
		{
			result[i * strnum + j] = (char*)malloc(sizeof(char) * stringSize);
			
			if(result[i * strnum + j] == NULL)
			{
				printf("Error while allocating memory\n");
				exit(0);
			}
			
			sprintf(result[i * strnum + j], "Thread %d %d", i, i * threads + j);
		}
	}
	return result;
}

void deleteStrings(char** strings, int threads, int strnum)
{
	int i;
	for(i = 0; i < threads * strnum; ++i)
	{
		free(strings[i]);
	}
	
	free(strings);
}

int main()
{
	pthread_t threads[numOfThreads];
	char **texts = createStrings(numOfThreads, numOfStrings);
	int code;
	int i;
	
	//Creating threads for printing strings
	for (i = 0; i < numOfThreads; ++i)
	{
		code = pthread_create(&threads[i], NULL, threadBody, (void*)(&texts[i * numOfStrings]));
		if (code != 0)
		{
			perror("Thread create error");
			exit(0);
		}
	}
	
	//Joining threads 
	for (i = 0; i < numOfThreads; ++i)
	{
		code = pthread_join(threads[i], NULL);
		if (code != 0)
		{
			perror("Joing thread error:");
		}
	}
	
	
	deleteStrings(texts, numOfThreads, numOfStrings);
	
	pthread_exit(NULL);
}