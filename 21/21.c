#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#define PHILO 5
#define DELAY 30000
#define FOOD 50

#define SUCCESS 0
#define ERROR -1
#define FORKS_TAKEN 1
#define FORKS_PUTTED 0

pthread_mutex_t forks[PHILO];
pthread_mutex_t foodLock;

pthread_mutex_t forkLock;
pthread_cond_t forkCond;

int lockMutex(pthread_mutex_t * mu)
{
	errno = pthread_mutex_lock(mu);
	if(errno != SUCCESS)
	{
		perror("Unable to lock mutex");
	}
	return errno;
}

int unlockMutex(pthread_mutex_t * mu)
{
	errno = pthread_mutex_unlock(mu);
	if(errno != SUCCESS)
	{
		perror("Unable to unlock mutex");
	}
	return errno;
}

int trylockMutex(pthread_mutex_t * mu)
{
	int eCode;
	eCode = pthread_mutex_trylock(mu);
	if(eCode != SUCCESS && eCode != EBUSY)
	{
		errno = eCode;
		perror("Unable to trylock mutex");
	}
	return eCode;
}

int condWait(pthread_cond_t * c, pthread_mutex_t * m)
{
	int eCode;
	eCode = pthread_cond_wait(c, m);
	if(eCode != SUCCESS)
	{
		errno = eCode;
		perror("Unable to cond wait");
	}
	return eCode;
}

int condBroadcast(pthread_cond_t * c)
{
	int eCode = pthread_cond_broadcast(c);
	if(eCode != SUCCESS)
	{
		errno = eCode;
		perror("Unable to pthread_cond_broadcast");
	}
	return eCode;
}

int lockTwoForks(int firstFork, int secondFork)
{
	int eCode;
	while(1)
	{
		eCode = trylockMutex(&forks[firstFork]);
		if(eCode != SUCCESS)
		{
			if(eCode != EBUSY) {	return eCode;	}
			
			eCode = condWait(&forkCond, &forkLock);
			if(eCode != SUCCESS) {	return eCode;	}
			
			continue;
		}

		eCode = trylockMutex(&forks[secondFork]);
		if(eCode != SUCCESS)
		{
			if(eCode != EBUSY)
			{
				unlockMutex(&forks[firstFork]);
				return eCode;
			}

			eCode = unlockMutex(&forks[firstFork]);
			if(eCode != SUCCESS) {	return eCode;	}
			
			eCode = condBroadcast(&forkCond);
			if(eCode != SUCCESS) {	return eCode;	}
			
			eCode = condWait(&forkCond, &forkLock);
			if(eCode != SUCCESS) {	return eCode;	}
			
			continue;
		}
		break;
	}
	return SUCCESS;
}

int getForks(int firstFork, int secondFork)
{
	int eCode = lockMutex(&forkLock);
	if(eCode != SUCCESS) {		return eCode;	}
	
	eCode = lockTwoForks(firstFork, secondFork);
	if(eCode != SUCCESS)
	{
		unlockMutex(&forkLock);
		return eCode;
	}

	eCode = unlockMutex(&forkLock);
	if(eCode != SUCCESS)
	{
		unlockMutex(&forks[firstFork]);
		unlockMutex(&forks[secondFork]);
		return eCode;
	}
	
	return SUCCESS;
}

int downForks(int firstFork, int secondFork)
{
	if(lockMutex(&forkLock) != SUCCESS)
	{
		return ERROR;
	}
	if(unlockMutex(&forks[firstFork]) != SUCCESS)
	{
		unlockMutex(&forks[secondFork]);
		condBroadcast(&forkCond);
		unlockMutex(&forkLock);
		return ERROR;
	}
	if(unlockMutex(&forks[secondFork]) != SUCCESS)
	{
		condBroadcast(&forkCond);
		unlockMutex(&forkLock);
		return ERROR;
	}
	if(condBroadcast(&forkCond) != SUCCESS)
	{
		unlockMutex(&forkLock);
		return ERROR;
	};
	
	if(unlockMutex(&forkLock) != SUCCESS)
	{
		return ERROR;
	}

	return SUCCESS;
}

int getFood()
{
	static int food = FOOD;
	int myFood;

	errno = lockMutex(&foodLock);
	if(errno != SUCCESS)
	{
		return ERROR;
	}

	myFood = food;

	if(food > 0)
		--food;

	errno = unlockMutex(&foodLock);
	if(errno != SUCCESS)
	{
		return ERROR;
	}
	return myFood;
}

struct resourses
{
	int firstFork;
	int secondFork;
	int forksTaken;
};

void cancelPhiloHandler(void * arg)
{
	struct resourses* res = (struct resourses*)arg;
	
	if(res->forksTaken == FORKS_TAKEN)
	{
		downForks(res->firstFork, res->secondFork);
	}
	res->forksTaken = FORKS_PUTTED;
}


int eatLoop(int id, struct resourses* res)
{
	int eaten = 0,
		food;
	while(1)
	{
		if(getForks(res->firstFork, res->secondFork) != SUCCESS) {pthread_exit(NULL); }

		res->forksTaken = FORKS_TAKEN;
		food = getFood();
		if(food == ERROR || food == 0)
		{
			if(downForks(res->firstFork, res->secondFork) != SUCCESS) { pthread_exit(NULL); };
			if(food == 0) break;
			if(food == ERROR) pthread_exit(NULL);
		}

		++eaten;

		printf("Philo %d: get dish %d.\n", id, food);

		usleep(DELAY * (FOOD - food + 1));

		if(downForks(res->firstFork, res->secondFork) != SUCCESS) { pthread_exit(NULL); };
		res->forksTaken = FORKS_PUTTED;
		usleep(DELAY * (FOOD - food + 1));
	}
	
	return eaten;
}

void * philosopher(void* num)
{
	int id = (int)num;
	int firstFork = id, 
		secondFork = (id + 1) % PHILO,
		eaten;
	
	printf("Philosopher %d sitting down to dinner.\n", id);

	struct resourses res = {firstFork, secondFork, FORKS_PUTTED};
	
	pthread_cleanup_push(cancelPhiloHandler, (void*)(&res));
	
	eaten = eatLoop(id, &res);
	
	pthread_cleanup_pop(0);

	printf("Philo %d is done eating. Total eaten: %d.\n", id, eaten);
	pthread_exit(NULL);
}


void initCond()
{
	int eCode = pthread_cond_init(&forkCond, NULL);
	if(eCode != SUCCESS)
	{
		errno = eCode;
		perror("Unable to init fork cond");
		exit(eCode);
	}
}

void destroyCond()
{
	int eCode = pthread_cond_destroy(&forkCond);
	if(eCode != SUCCESS)
	{
		errno = eCode;
		perror("Unable to destroy fork cond");
	}
}

void destroyMutex(pthread_mutex_t* mu)
{
	int eCode = pthread_mutex_destroy(mu);
	if(eCode != SUCCESS)
	{
		errno = eCode;
		perror("Unable to destroy mutex");
	}
}

void destroyMutexes(int num)
{
	destroyMutex(&foodLock);
	
	destroyMutex(&forkLock);

	int i;
	for(i = 0; i < num; ++i)
	{
		destroyMutex(&forks[i]);
	}
}

void initMutexes()
{
	int eCode = pthread_mutex_init(&foodLock, NULL);
	if(eCode != SUCCESS)
	{
		errno = eCode;
		perror("Unable to init food lock");
		destroyCond();
		exit(eCode);
	}
	
	eCode = pthread_mutex_init(&forkLock, NULL);
	if(eCode != SUCCESS)
	{
		errno = eCode;
		perror("Unable to init eat lock");
		destroyMutex(&foodLock);
		destroyCond();
		exit(eCode);
	}
	
	int i;
	for(i = 0; i < PHILO; ++i)
	{
		errno = pthread_mutex_init(&forks[i], NULL);
		if(errno != SUCCESS)
		{
			int eCode = errno;
			perror("Unable to init fork lock");
			destroyMutexes(i);
			exit(eCode);
		}
	}
}

void cancelPhils(pthread_t* phils, int num)
{
	int i;
	for(i = 0; i < num; ++i)
	{
		errno = pthread_cancel(phils[i]);
		if(errno != SUCCESS)
		{
			perror("Unable to cancel philo");
			continue;
		}

		errno = pthread_join(phils[i], NULL);
		if(errno != SUCCESS)
		{
			perror("Unable to join philo");
		}
	}
}

int main(int argc, char* argv[])
{
	int i;
	pthread_t phils[PHILO];

	initCond();
	initMutexes();

	int eCode;
	
	for(i = 0; i < PHILO; ++i)
	{
		eCode = pthread_create(&phils[i], NULL, philosopher, (void*)i);
		if(eCode != SUCCESS)
		{
			errno = eCode;
			perror("Unable to create philosopher");
			cancelPhils(phils, i);
			destroyMutexes(PHILO);
			exit(eCode);
		}
	}

	for(i = 0; i < PHILO; ++i)
	{
		eCode = pthread_join(phils[i], NULL);
		if(eCode != SUCCESS)
		{
			errno = eCode;
			perror("Unable to join philosopher");
		}
	}

	destroyMutexes(PHILO);

	return SUCCESS;
}