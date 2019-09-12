#include <sys/types.h>
#include <sys/ddi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <strings.h>

#define numOfIterations 2000000

void * threadBody(void * param) 
{
	double * result = (double*)malloc(sizeof(double));
	*result = 0;
	int i;
	int begin = ((int*)param)[0];
	int end = ((int*)param)[1];
	for(i = begin; i < end; ++i)
	{
		*result += 1.0/(i*4.0 + 1.0);
		*result -= 1.0/(i*4.0 + 3.0);
	}
	return (void*)result;
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
	
	int* shifts = (int*)malloc(sizeof(int) * (numOfThreads + 1));
	
	int i;
	for (i = 0; i < numOfThreads; ++i)
	{
		shifts[i] = numOfIterations/numOfThreads * i;
	}
	shifts[numOfThreads] = numOfIterations;
	
	for (i = 0; i < numOfThreads; ++i)
	{
		code = pthread_create(&threads[i], NULL, threadBody, (void*)(&shifts[i]));
		if (code != 0)
		{
			perror("Thread create error");
			exit(0);
		}
	}
	
	double pi = 0;
	for(i = 0; i < numOfThreads; ++i)
	{
		double* result;
		code = pthread_join(threads[i], (void**)(&result));
		if (code != 0){
			perror("Joing thread error");
			exit(0);
		}
		pi += *result;
	}
	pi *= 4.0;
	printf("pi done - %.15g \n", pi);  
	
	return 0;
}
