#include <sys/types.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <semaphore.h>

#define SUCCESS 0
#define NUM_OF_SEMS 2
#define MASTER_SEM 0
#define SLAVE_SEM 1

sem_t sems[NUM_OF_SEMS];

int semWait(sem_t* sem)
{
	int eCode = sem_wait(sem);
	if(eCode != SUCCESS)
	{
		perror("Unable to sem_wait");
	}
	return eCode;
}

int semPost(sem_t* sem)
{
	int eCode = sem_post(sem);
	if(eCode != SUCCESS)
	{
		perror("Unable to sem_post");
	}
	return eCode;
}

void printTenStrings(const char * text, int semToPost, int semToWait) 
{
	int i;
	for (i = 0; i < 10; ++i)
	{
		if(semWait(&sems[semToWait]) != SUCCESS) {	pthread_exit(NULL);	}
		printf("%d: %s\n", i + 1, text);
		if(semPost(&sems[semToPost]) != SUCCESS) {	pthread_exit(NULL); }
	}
}

void * threadBody(void * param) 
{
	printTenStrings("I'm thread", MASTER_SEM, SLAVE_SEM);
}

void destroySems(int end)
{
	int i;
	for(i = 0; i < end; ++i)
	{
		int eCode = sem_destroy(&sems[i]);
		if(eCode != SUCCESS)
		{
			perror("Unable to destoy semaphore");
		}
	}
}

void initSems()
{
	int eCode = sem_init(&sems[MASTER_SEM], 0, 1);
	if(eCode != SUCCESS)
	{
		perror("Unable to init master semaphore");
		exit(eCode);
	}
	
	eCode = sem_init(&sems[SLAVE_SEM], 0, 0);
	if(eCode != SUCCESS)
	{
		perror("Unable to init slave semaphore");
		destroySems(SLAVE_SEM);
		exit(eCode);
	}
}


int createThread(pthread_t* thread)
{
	int eCode = pthread_create(thread, NULL, threadBody, NULL);
	if (eCode != SUCCESS)
	{
		errno = eCode;
		perror("Unable to create slave thread");
	}
	return eCode;
}

int main()
{
	pthread_t thread;

	initSems();

	int eCode = createThread(&thread);
	if(eCode != SUCCESS)
	{
		destroySems(NUM_OF_SEMS);
		exit(eCode);
	}

	printTenStrings("yes", SLAVE_SEM, MASTER_SEM);

	eCode = pthread_join(thread, NULL);
	if(eCode != SUCCESS)
	{
		errno = eCode;
		perror("Joining thread error");
		destroySems(NUM_OF_SEMS);
		exit(eCode);
	}

	destroySems(NUM_OF_SEMS);
	return 0;
}