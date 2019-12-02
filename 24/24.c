#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define CONSUMERS_NUM 2
#define PRODUCERS_NUM 2
#define CAPACITY 10
#define MAX_LEN 80
#define SUCCESS 0
#define QUEUE_DROPPED 1
#define QUEUE_NOT_DROPPED 0
#define EQDROP 0
#define ERROR -1

#define WORKTIME 10
#define DELAY 1000

typedef
struct queue
{
	pthread_mutex_t mutex;
	pthread_cond_t filledCond;
	pthread_cond_t emptyCond;
	char** messages;
	int capacity;
	int dropped;
	int head;
	int tail;
	int size;
} queue;

int mutexLock(pthread_mutex_t * mu)
{
	errno = pthread_mutex_lock(mu);
	if(errno != SUCCESS)
	{
		perror("Unable to lock mutex");
	}
	return errno;
}

int mutexUnlock(pthread_mutex_t * mu)
{
	errno = pthread_mutex_unlock(mu);
	if(errno != SUCCESS)
	{
		perror("Unable to unlock mutex");
	}
	return errno;
}

int mutexInit(pthread_mutex_t* mu)
{
	errno = pthread_mutex_init(mu, NULL);
	if(errno != SUCCESS)
	{
		perror("Unable to init mutex");
	}
	return errno;
}

void mutexDestroy(pthread_mutex_t* mu)
{
	errno = pthread_mutex_destroy(mu);
	if(errno != SUCCESS)
	{
		perror("Unable to init mutex");
	}
}

int condInit(pthread_cond_t* c)
{
	errno = pthread_cond_init(c, NULL);
	if(errno != SUCCESS)
	{
		perror("Unable to init cond");
	}
	return errno;
}

void condDestroy(pthread_cond_t* c)
{
	int eCode = pthread_cond_destroy(c);
	if(eCode != SUCCESS)
	{
		errno = eCode;
		perror("Unable to destroy eat cond");
	}
}

int condSignal(pthread_cond_t* c)
{
	errno = pthread_cond_signal(c);
	if(errno != SUCCESS)
	{
		perror("Unable to cond signal");
	}
	return errno;
}

int condBroadcast(pthread_cond_t* c)
{
	errno = pthread_cond_broadcast(c);
	if(errno != SUCCESS)
	{
		perror("Unable to cond signal");
	}
	return errno;
}

int condWait(pthread_cond_t* c, pthread_mutex_t* m)
{
	errno = pthread_cond_wait(c, m);
	if(errno != SUCCESS)
	{
		perror("Unable to cond wait");
	}
	
	return errno;
}


char** createMessages(int capacity, int maxLen)
{
	char** result = (char**)malloc(sizeof(char*) * capacity);
	if(result == NULL)
	{
		perror("Unable allocate memory for messages array");
		return result;
	}
	
	int i;
	for(i = 0; i < capacity; ++i)
	{
		result[i] = (char*)malloc(sizeof(char) * maxLen);
		if(result[i] == NULL)
		{
			perror("Unable allocate memory for string");
			int j;
			for(j = 0; j < i; ++j)
			{
				free(result[j]);
			}
			free(result);
			return NULL;
		}
	}
	
	return result;
}

queue * queueCtor(int _capacity)
{
	queue* result = (queue*)malloc(sizeof(queue));
	if(result == NULL)
	{
		perror("Unable to allocate memory for queue");
		return NULL;
	}
	
	errno = mutexInit(&result->mutex);
	if(errno != SUCCESS)
	{
		free(result);
		return NULL;
	}
	
	errno = condInit(&result->filledCond);
	if(errno != SUCCESS)
	{
		mutexDestroy(&result->mutex);
		free(result);
		return NULL;
	}
	
	errno = condInit(&result->emptyCond);
	if(errno != SUCCESS)
	{
		mutexDestroy(&result->mutex);
		condDestroy(&result->filledCond);
		free(result);
		return NULL;
	}
	
	result->capacity = _capacity;
	result->head = 0;
	result->tail = 0;
	result->dropped = QUEUE_NOT_DROPPED;
	result->size = 0;
	
	result->messages = createMessages(result->capacity, MAX_LEN);
	if(result->messages == NULL)
	{
		mutexDestroy(&result->mutex);
		condDestroy(&result->filledCond);
		condDestroy(&result->emptyCond);
		free(result);
		return NULL;
	}
	
	return result;
}

void queueDtor(queue** q)
{
	queue* qu = (*q);
	
	mutexDestroy(&qu->mutex);
	condDestroy(&qu->filledCond);
	condDestroy(&qu->emptyCond);
	
	int i;
	for(i = 0; i < qu->capacity; ++i)
	{
		free(qu->messages[i]);
	}
	
	free(qu->messages);
	
	free(qu);
	
	*q = NULL;
}

int queueDrop(queue * q)
{
	int eCode = mutexLock(&q->mutex);
	if(eCode != SUCCESS)
	{
		return eCode;
	}
	
	q->dropped = QUEUE_DROPPED;
	q->size = -1;
	
	eCode = condBroadcast(&q->filledCond);
	if(eCode != SUCCESS)
	{
		mutexUnlock(&q->mutex);
		return eCode;
	}
	
	eCode = condBroadcast(&q->emptyCond);
	if(eCode != SUCCESS)
	{
		mutexUnlock(&q->mutex);
		return eCode;
	}
	
	eCode = mutexUnlock(&q->mutex);
	return eCode;
}

int queuePut(queue * q, char* mess)
{
	if(mutexLock(&q->mutex) != SUCCESS) { return ERROR; }
	
	if(q->dropped == QUEUE_DROPPED)
	{
		if(mutexUnlock(&q->mutex) != SUCCESS) {	return ERROR; }
		return EQDROP;
	}
	
	while(q->size == q->capacity)
	{
		if(condWait(&q->emptyCond, &q->mutex) != SUCCESS)
		{
			mutexUnlock(&q->mutex);
			return ERROR;
		}
	}
	
	if(q->dropped == QUEUE_DROPPED)
	{
		if(mutexUnlock(&q->mutex) != SUCCESS) {	return ERROR; }
		return EQDROP;
	}
	
	int result = strlcpy(q->messages[q->tail], mess, MAX_LEN);
	if(result >= MAX_LEN)
	{
		mutexUnlock(&q->mutex);
		return ERROR;
	}

	q->tail = (q->tail + 1) % q->capacity;
	q->size++;
	
	int eCode = condSignal(&q->filledCond);
	if(eCode != SUCCESS)
	{
		mutexUnlock(&q->mutex);
		return ERROR;
	}
	
	if(mutexUnlock(&q->mutex) != SUCCESS) return ERROR;
	return result;
}

int queuePop(queue* q, char* buf, size_t buf_size)
{
	if(mutexLock(&q->mutex) != SUCCESS) {	return ERROR;	}
	
	if(q->dropped == QUEUE_DROPPED)
	{
		if(mutexUnlock(&q->mutex) != SUCCESS) {	return ERROR;	}
		return EQDROP;
	}
	
	while(q->size == 0)
	{
		if(condWait(&q->filledCond, &q->mutex) != SUCCESS)
		{
			mutexUnlock(&q->mutex);
			return ERROR;
		}
	}
	
	if(q->dropped == QUEUE_DROPPED)
	{
		if(mutexUnlock(&q->mutex) != SUCCESS) {	return ERROR; }
		return EQDROP;
	}
	
	int result = strlcpy(buf, q->messages[q->head], buf_size);
	if(result >= buf_size)
	{
		mutexUnlock(&q->mutex);
		return ERROR;
	}

	q->head = (q->head + 1) % q->capacity;
	q->size--;
	
	int eCode = condSignal(&q->emptyCond);
	if(eCode != SUCCESS)
	{
		mutexUnlock(&q->mutex);
		return ERROR;
	}
	
	if(mutexUnlock(&q->mutex) != SUCCESS) { return ERROR; }
	return result;
}

queue* messageQueue;

void *producer(void * args)
{
	int id = (int)args;
	char buffer[MAX_LEN];
	
	snprintf(buffer, MAX_LEN, "Producer #%d string", id);
	
	while(1)
	{
		int result = queuePut(messageQueue, buffer);
		if(result == EQDROP || result == ERROR)
		{
			break;
		}
		usleep((rand() % DELAY) * DELAY);
	}
}

void *consumer(void * args)
{
	int id = (int)args;
	char buffer[MAX_LEN];
	
	while(1)
	{
		int result = queuePop(messageQueue, buffer, MAX_LEN);
		if(result == EQDROP || result == ERROR)
		{
			break;
		}
		printf("Consumer #%d consume message: %s\n", id, buffer);
		buffer[0] = '\0';
		usleep((rand() % DELAY) * DELAY);
	}
}


void termitateThreads(pthread_t * threads, int num)
{
	queueDrop(messageQueue);
	
	int eCode;
	
	int i;
	for(i = 0; i < num; ++i)
	{
		eCode = pthread_join(threads[i], NULL);
		if(eCode != SUCCESS)
		{
			errno = eCode;
			perror("Unable to join thread");
		}
	}
}

int createConsumers(pthread_t* threads, int consNum, int prodNum)
{
	int eCode;
	int i;
	for(i = prodNum; i < consNum + prodNum; ++i)
	{
		eCode = pthread_create(&threads[i], NULL, consumer, (void*)(i - prodNum + 1));
		if(eCode != SUCCESS)
		{
			errno = eCode;
			perror("Unable to create consumer");
			termitateThreads(threads, i);
			queueDtor(&messageQueue);
			return ERROR;
		}
	}
	return SUCCESS;
}

int createProducers(pthread_t* threads, int prodNum)
{
	int eCode;
	int i;
	for(i = 0; i < prodNum; ++i)
	{
		eCode = pthread_create(&threads[i], NULL, producer, (void*)(i + 1));
		if(eCode != SUCCESS)
		{
			errno = eCode;
			perror("Unable to create producer");
			termitateThreads(threads, i);
			return ERROR;
		}
	}
	return SUCCESS;
}

int main()
{
	pthread_t threads[CONSUMERS_NUM + PRODUCERS_NUM];
	
	messageQueue = queueCtor(CAPACITY);
	if(messageQueue == NULL)
		return ERROR;
	
	int eCode;

	eCode = createProducers(threads, PRODUCERS_NUM);
	if(eCode != SUCCESS)
	{
		queueDtor(&messageQueue);
		return ERROR;
	}
	
	eCode = createConsumers(threads, CONSUMERS_NUM, PRODUCERS_NUM);
	if(eCode != SUCCESS)
	{
		queueDtor(&messageQueue);
		return ERROR;
	}
	
	sleep(WORKTIME);
	
	eCode = queueDrop(messageQueue);
	if(eCode != SUCCESS)
		return ERROR;
	
	termitateThreads(threads, CONSUMERS_NUM + PRODUCERS_NUM);
	queueDtor(&messageQueue);
	
	return 0;
}


	// int i;
	// for(i = 0; i < CONSUMERS_NUM; ++i)
	// {
		// eCode = pthread_create(&threads[i], NULL, consumer, (void*)(i + 1));
		// if(eCode != SUCCESS)
		// {
			// errno = eCode;
			// perror("Unable to create consumer");
			// termitateThreads(threads, i);
			// queueDtor(&messageQueue);
			// return ERROR;
		// }
	// }
	
	// for(i = CONSUMERS_NUM; i < CONSUMERS_NUM + PRODUCERS_NUM; ++i)
	// {
		// eCode = pthread_create(&threads[i], NULL, producer, (void*)(i - CONSUMERS_NUM + 1));
		// if(eCode != SUCCESS)
		// {
			// errno = eCode;
			// perror("Unable to create consumer");
			// termitateThreads(threads, i);
			// queueDtor(&messageQueue);
			// return ERROR;
		// }
	// }







