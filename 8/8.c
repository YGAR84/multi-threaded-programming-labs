#include <sys/types.h>
#include <sys/ddi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <strings.h>
#include <signal.h>

#define timer 1000000

int flag = 0;
int numOfThreads;

struct data {
	double val;
	int rest;
};

void * threadBody(void * param)
{
	struct data * datas = (struct data*)param;
	int i = 0;
	while(1)
	{
		if(i % timer == 0 && flag != 0)
		{
			break;
		}
		datas->val += 1.0/((i * numOfThreads + datas->rest) * 4.0 + 1.0);
		datas->val -= 1.0/((i * numOfThreads + datas->rest) * 4.0 + 3.0);
		++i;
	}
	pthread_exit(NULL);
}

void sigcatch(int sig)
{
	flag = 1;
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		printf("There is no num of threads\n");
		exit(0);
	}
	int code;
	numOfThreads = atoi(argv[1]);
	printf("num of threads = %d\n", numOfThreads);
	
	pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * numOfThreads);
	struct data * datas = (struct data *)malloc(sizeof(struct data) * numOfThreads);
	
	int i;
	for(i = 0; i < numOfThreads; ++i)
	{
		datas[i].rest = i;
		datas[i].val = 0;
	}
	
	for (i = 0; i < numOfThreads; ++i)
	{
		code = pthread_create(&threads[i], NULL, threadBody, (void*)(&datas[i]));
		if (code != 0)
		{
			perror("Thread create error");
			exit(0);
		}
	}
	
	signal(SIGINT, sigcatch);
	
	double pi = 0;
	for(i = 0; i < numOfThreads; ++i)
	{
		code = pthread_join(threads[i], NULL);
		if (code != 0){
			perror("Joing thread error");
			exit(0);
		}
		pi += datas[i].val;
	}
	pi *= 4.0;
	printf("pi done - %.15g \n", pi);  
	
	free(datas);
	free(threads);
	return 0;
}
