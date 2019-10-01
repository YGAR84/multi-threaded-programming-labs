#include <sys/types.h>
#include <sys/ddi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <strings.h>

#define numOfIterations 2000000

struct data {
	int begin, end;
	double val;
};

void * threadBody(void * param) 
{
	struct data * datas = (struct data*)param;
	int i;
	for(i = datas->begin; i < datas->end; ++i)
	{
		datas->val += 1.0/(i*4.0 + 1.0);
		datas->val -= 1.0/(i*4.0 + 3.0);
	}
	pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		printf("There is no num of threads\n");
		exit(0);
	}
	int code;
	int numOfThreads = atoi(argv[1]);
	printf("num of threads = %d\n", numOfThreads);
	
	pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * numOfThreads);
	struct data * datas = (struct data*)malloc(sizeof(struct data) * numOfThreads);
	
	int i;
	for (i = 0; i < numOfThreads; ++i)
	{
		datas[i].val = 0; 
		datas[i].begin = numOfIterations/numOfThreads * i;
		datas[i].end = numOfIterations/numOfThreads * (i + 1);
	}
	datas[numOfThreads - 1].end = numOfIterations;
	
	for (i = 0; i < numOfThreads; ++i)
	{
		code = pthread_create(&threads[i], NULL, threadBody, (void*)(&datas[i]));
		if (code != 0)
		{
			perror("Thread create error");
			exit(0);
		}
	}
	
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
	
	free(threads);
	free(datas);
	return 0;
}
