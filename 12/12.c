#include <sys/types.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define SUCCESS 0

#define MASTER_BREAK_VALUE 0
#define SLAVE_BREAK_VALUE 1

pthread_mutex_t mutex;
pthread_cond_t cond;

int flag = 0;

int lockMutex()
{
	errno = pthread_mutex_lock(&mutex);
	if(errno != SUCCESS)
	{
		perror("Unable to lock mutex");
	}
	return errno;
}

int unlockMutex()
{
	errno = pthread_mutex_unlock(&mutex);
	if(errno != SUCCESS)
	{
		perror("Unable to unlock mutex");
	}
	return errno;
}

void printTenStrings(const char * text, int breakValue) 
{
	int eCode = lockMutex();
	if(eCode != SUCCESS)
	{
		return;
	}
	
	int i;
	for (i = 0; i < 10; ++i)
	{
		while(flag != breakValue)
		{
			eCode = pthread_cond_wait(&cond, &mutex);
			if(eCode != SUCCESS)
			{
				errno = eCode;
				perror("Unable to pthread_cond_wait");
				unlockMutex();
				return;
			}
		}
		
		printf("%d: %s\n", i + 1, text);
		
		flag = (flag + 1) % 2;
		eCode = pthread_cond_signal(&cond);
		if(eCode != SUCCESS)
		{
			errno = eCode;
			perror("Unable to pthread_cond_wait");
			unlockMutex();
			return;
		}
	}
	
	unlockMutex();
}


void * threadBody(void * param) 
{
	int breakValue = (int)param;
	
	printTenStrings("I'm thread", breakValue);
}


void destroyCond()
{
	errno = pthread_cond_destroy(&cond);
	if(errno != SUCCESS)
	{
		perror("Unable to destroy condition");
	}
}

void destroyMutex()
{
	errno = pthread_mutex_destroy(&mutex);
	if(errno != SUCCESS)
	{
		perror("Unable to destroy mutex");
	}
}

void initMutex()
{
	int eCode = pthread_mutex_init(&mutex, NULL);
	if(eCode != SUCCESS)
	{
		errno = eCode;
		perror("Unable to create mutex");
		exit(eCode);
	}
}

void initCond()
{
	int eCode = pthread_cond_init(&cond, NULL);
	if(eCode != SUCCESS)
	{
		errno = eCode;
		perror("Unable to init condition");
		destroyMutex();
		exit(eCode);
	}
}

int createThread(pthread_t* thread, int breakValue)
{
	int eCode = pthread_create(thread, NULL, threadBody, (void*)breakValue);
	if (eCode != SUCCESS)
	{
		errno = eCode;
		perror("Unable to create slave thread");
		if(unlockMutex() != SUCCESS)
		{
			return eCode;
		}
		destroyMutex();
		destroyCond();
	}
	return eCode;
}

int main()
{
	pthread_t thread;
	
	initMutex();
	initCond();
	

	int eCode = createThread(&thread, SLAVE_BREAK_VALUE);
	if(eCode != SUCCESS)
	{
		exit(eCode);
	}
	
	printTenStrings("yes", MASTER_BREAK_VALUE);
	
	eCode = pthread_join(thread, NULL);
	if(eCode != SUCCESS)
	{
		errno = eCode;
		perror("Joining thread error");
		destroyMutex();
		destroyCond();
		exit(eCode);
	}
	
	destroyMutex();
	destroyCond();
	
	return 0;
}