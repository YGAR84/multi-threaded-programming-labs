#include <sys/types.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define numOfThreads 4
#define numOfStrings 4
#define stringSize 20

typedef
struct MemoryList
{
	void* ptr;
	struct MemoryList *prev;
} MemoryList;

MemoryList* memoryList;

MemoryList* getMemoryList()
{
	return memoryList;
}

void printMemoryList()
{
	MemoryList* iter = memoryList;
	printf("_____________\n");
	while(iter != NULL)
	{
		printf("|%-x|\n", iter->ptr);
		iter = iter->prev;
	}
	printf("_____________\n");
	return;
}

void freeMemoryList()
{
	MemoryList* iter = getMemoryList();
	while(iter != NULL)
	{
		free(iter->ptr);
		MemoryList* iter2 = iter->prev;
		free(iter);
		iter = iter2;
	}
	memoryList = NULL;
	return;
}

MemoryList* ctorMemoryList(void* _ptr)
{
	MemoryList *result = (MemoryList*)malloc(sizeof(MemoryList));
	if (result == NULL){
		freeMemoryList();
		return NULL;
	}
	result->ptr = _ptr;
	result->prev = NULL;
	return result;
}

MemoryList* addInMemoryList(void *ptr)
{
	MemoryList* ml = getMemoryList();
	if(ml == NULL)
	{
		ml = ctorMemoryList(ptr);
		if(ml == NULL)
			return NULL;
		memoryList = ml;
		
		return ml;
	}
	
	MemoryList* head = ctorMemoryList(ptr);
	if(head == NULL)
		return NULL;
	
	head->prev = ml;
	memoryList = head;
	return head;
}

void* myMalloc(size_t size)
{
	void* result = malloc(size);
	if (result == NULL)
	{
		freeMemoryList();
		fprintf(stderr, "Error while allocating memory\n");
		exit(1);
	}
	
	MemoryList* ml = addInMemoryList(result);
	if(ml == NULL)
	{
		fprintf(stderr, "Error while allocating memory\n");
		freeMemoryList();
		exit(1);
	}
	return result;
}

void myFree(void* _ptr)
{
	MemoryList* ml = getMemoryList();
	MemoryList* iter = memoryList;  
	if(iter == NULL)
	{
		return;
	}
	if(iter->ptr == _ptr)
	{
		free(iter->ptr);
		ml = iter->prev;
		free(iter);
		return;
	}
	
	MemoryList* iter2 = iter->prev;
	while(iter2 != NULL)
	{
		if(iter2->ptr = _ptr)
		{
			free(iter2->ptr);
			iter->prev = iter2->prev;
			free(iter2);
			return;
		}
		iter2 = iter2->prev;
		iter = iter->prev;
	}
	return;
}

void printStrings(char ** texts) 
{
	int i;
	for(i = 0; texts[i] != NULL; ++i)
	{
		printf("%d:%s\n", i + 1, texts[i]);
	}
	return;
}

void * threadBody(void * param) 
{
	printStrings((char**)param);
}

char*** createStrings(int threads, int strnum)
{
	char*** result = (char***)myMalloc(sizeof(char**) * threads);
	if(result == NULL)
	{
		return NULL;
	}

	int i;
	for(i = 0; i < threads; ++i)
	{
		result[i] = (char**)myMalloc(sizeof(char*) * (strnum + 1));
		
		int j;
		for(j = 0; j < strnum; ++j)
		{
			result[i][j] = (char*)myMalloc(sizeof(char)*stringSize);
			sprintf(result[i][j], "Thread %d %d", i, i * threads + j);
		}
		result[i][strnum] = NULL;
	}
	return result;
}

int main()
{
	pthread_t threads[numOfThreads];
	char*** texts = createStrings(numOfThreads, numOfStrings);
	
	int code;
	int i;
	
	//Creating threads for printing strings
	for (i = 0; i < numOfThreads; ++i)
	{
		code = pthread_create(&threads[i], NULL, threadBody, (void*)(texts[i]));
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
	
	
	freeMemoryList();
	pthread_exit(NULL);
}