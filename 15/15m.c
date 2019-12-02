#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define SUCCESS 0
#define ERROR -1
#define NUM_OF_SEMS 2
#define MASTER_SEM 0
#define SLAVE_SEM 1

sem_t* sems[NUM_OF_SEMS];
char* semsNames[NUM_OF_SEMS] = { "/sem_master", "/sem_slave"};

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
		if(semWait(sems[semToWait]) != SUCCESS) return;
		printf("%d: %s\n", i + 1, text);
		if(semPost(sems[semToPost]) != SUCCESS) return;
	}
}

void closeSems(int end)
{
	int i;
	for(i = 0; i < end; ++i)
	{
		int eCode = sem_close(sems[i]);
		if(eCode != SUCCESS)
		{
			perror("Unable to close semaphore");
		}
		eCode = sem_unlink(semsNames[i]);
		if(eCode != SUCCESS)
		{
			perror("Unable to unlink semaphore");
		}
	}
}


void openSems()
{
	int i;
	for(i = 0; i < NUM_OF_SEMS; ++i)
	{
		sems[i] = sem_open(semsNames[i], O_CREAT, S_IRWXU, NUM_OF_SEMS - i - 1);
		if(sems[i] == SEM_FAILED)
		{
			perror("Unable to open semaphore");
			int eCode = errno;
			closeSems(i);
			exit(eCode);
		}
	}
}

int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		fprintf(stderr, "Not enough args(slave).\n");
		exit(ERROR);
	}
	openSems();
	
	int pid = fork();
	switch(pid)
	{
		case ERROR :
		{
			perror("Unable to fork");
			closeSems(NUM_OF_SEMS);
			exit(errno);
		}
		case SUCCESS :
		{
			execl(argv[1], NULL);
			perror("Unable to execl");
			closeSems(NUM_OF_SEMS);
			exit(errno);
		}
	}
	printTenStrings("Master", SLAVE_SEM, MASTER_SEM);
	
	int eCode = wait(NULL);
	if(eCode == ERROR)
	{
		return ERROR;
	}
	closeSems(NUM_OF_SEMS);
	return 0;
}