#include <sys/types.h>
#include <sys/ddi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <strings.h>
#include <signal.h>

#define timer 10000000

int flag = 0;

struct data {
	double val;
	int rest;
	int step;
};

void * threadBody(void * param)
{
	struct data * datas = (struct data*)param;
	int i = 0;
	while(1)
	{
		pthread_testcancel();
		if(i % timer == 0 && flag != 0)
		{
			break;
		}
		datas->val += 1.0/((i * datas->step + datas->rest) * 4.0 + 1.0);
		datas->val -= 1.0/((i * datas->step + datas->rest) * 4.0 + 3.0);
		++i;
	}
	
	pthread_exit(NULL);
}

void sigcatch(int sig)
{
	flag = 1;
}

int getNumOfThreads(int argc, char* argv[])
{
	if (argc < 2)
	{
		printf("There is no num of threads\n");
		exit(0);
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
		exit(0);
	}
	
	*datas = (struct data*)malloc(sizeof(struct data) * numOfThreads);
	if(*datas == NULL)
	{
		fprintf(stderr, "Error while allocating memory\n");
		free(threads);
		exit(0);
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
	for(i = 0; i < numOfThreads; ++i)
	{
		datas[i].rest = i;
		datas[i].val = 0.0;
		datas[i].step = numOfThreads;
	}
	return;
}

void termitateThreads(pthread_t* threads, int begin, int end)
{
	int i;
	for(i = begin; i < end; ++i)
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
	if(numOfThreads == 0)
	{
		fprintf(stderr, "Invalid num of threads: %d.\n", numOfThreads);
		exit(1);
	}
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
			termitateThreads(threads, 0, i);
			exit(errno);
		}
	}
	
	signal(SIGINT, sigcatch);
	
	double pi = 0;
	for(i = 0; i < numOfThreads; ++i)
	{
		errno = pthread_join(threads[i], NULL);
		if (errno != 0){
			perror("Joing thread error");
			termitateThreads(threads, i + 1, numOfThreads);
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
