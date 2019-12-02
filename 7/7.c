#include <sys/types.h>
#include <sys/ddi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <strings.h>

#define numOfIterations 20000000

struct data 
{
	int begin, end;
	double val;
};

void * threadBody(void * param) 
{
	struct data * datas = (struct data*)param;
	int i;
	for(i = datas->begin; i < datas->end; ++i)
	{
		datas->val += 1.0/(i * 4.0 + 1.0);
		datas->val -= 1.0/(i * 4.0 + 3.0);
	}
	pthread_exit(NULL);
}

int getNumOfThreads(int argc, char* argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "There is no num of threads\n");
		exit(1);
	}

	int numOfThreads = atoi(argv[1]);
	return numOfThreads;
}

void dataInit(pthread_t** threads, struct data ** datas, int numOfThreads)
{
	*threads = (pthread_t*)malloc(sizeof(pthread_t) * numOfThreads);
	if(*threads == NULL)
	{
		fprintf(stderr, "Error while allocating memory\n");
		exit(1);
	}
	
	*datas = (struct data*)malloc(sizeof(struct data) * numOfThreads);
	if(*datas == NULL)
	{
		fprintf(stderr, "Error while allocating memory\n");
		free(threads);
		exit(1);
	}
	return;
}

void dataDelete(pthread_t** threads, struct data ** datas)
{
	free(*threads);
	free(*datas);
	*threads = NULL;
	*datas = NULL;
}

void fillDatas(struct data* datas, int numOfThreads)
{
	int i;
	int coef = numOfIterations/numOfThreads;
	for (i = 0; i < numOfThreads; ++i)
	{
		datas[i].val = 0; 
		datas[i].begin = coef * i;
		datas[i].end = coef * (i + 1);
	}
	datas[numOfThreads - 1].end = numOfIterations;
	return;
}


void termitateThreads(pthread_t* threads, int j)
{
	int i;
	for(i = 0; i < j; ++i)
	{
		errno = pthread_cancel(threads[i]);
		if(errno != 0)
		{
			perror("Error cancelling thread");
			continue;
		}
		void* status;
		errno = pthread_join(threads[i], &status);
		if(errno != 0)
		{
			perror("Error joining thread");
			continue;
		}
		if(status != PTHREAD_CANCELED)
		{
			fprintf(stderr, "Thread not canceled\n");
		}
	}
}

int main(int argc, char* argv[])
{
	int numOfThreads = getNumOfThreads(argc, argv);
	printf("num of threads = %d\n", numOfThreads);
	
	pthread_t* threads;
	struct data * datas;
	
	dataInit(&threads, &datas, numOfThreads);
	fillDatas(datas, numOfThreads);
	
	int i;
	for (i = 0; i < numOfThreads; ++i)
	{
		errno = pthread_create(&threads[i], NULL, threadBody, (void*)(&datas[i]));
		if (errno != 0)
		{
			perror("Thread create error");
			termitateThreads(threads, i);
			dataDelete(&threads, &datas);
			exit(errno);
		}
	}
	
	double pi = 0;
	for(i = 0; i < numOfThreads; ++i)
	{
		errno = pthread_join(threads[i], NULL);
		if (errno != 0)
		{
			perror("Joing thread error");
			termitateThreads(threads, i);
			dataDelete(&threads, &datas);
			exit(errno);
		}
		pi += datas[i].val;
	}
	pi *= 4.0;
	printf("pi done - %.15g \n", pi);
	
	dataDelete(&threads, &datas);
	
	return 0;
}