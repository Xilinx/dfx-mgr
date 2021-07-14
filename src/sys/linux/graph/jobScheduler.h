#include <pthread.h>

typedef struct JobScheduler JobScheduler_t; 
typedef struct JobQueueBuffer JobQueueBuffer_t; 
typedef struct queue queue_t; 
typedef struct Element Element_t; 

struct JobScheduler{
	queue_t* CommandQueue;
	queue_t* ResponseQueue;
	pthread_t thread[1];
	Element_t *graphList; 
};

struct JobQueueBuffer{
	int type;
	int id;
};

extern JobScheduler_t * jobSchedulerInit();
extern int jobSchedulerSubmit(JobScheduler_t *scheduler, Element_t *graphElement);
extern int jobSchedulerRemove(JobScheduler_t *scheduler, Element_t *graphElement);
