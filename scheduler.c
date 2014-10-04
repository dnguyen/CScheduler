#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

struct _thread {
    int id;
    float arrival_time;
    int required_time;
    int priority;
};
typedef struct _thread Thread;

/** Implement the READY queue as a singly linked list **/
typedef struct _queue_node Node;
struct _queue_node {
    Thread *thread;
    Node *next;
};

struct queue {
    Node *front;
    Node *back;
    int size;
};
typedef struct queue Queue;

FILE *logger;
Queue *ReadyQueue;
float CURRENT_TIME;
// Will probably need a pthread_cond?

void push(Queue *queue, Node *node) {
    fprintf(logger, "ADDED TO READY QUEUE [size=%d]\n", queue->size);
    fprintf(logger, "\t tid=%d arrival_time=%f required_time=%d priorty=%d\n",
        node->thread->id, node->thread->arrival_time, node->thread->required_time, node->thread->priority);

    if (queue->size == 0) {
        queue->front = node;
        queue->back = node;
    } else {
        queue->back->next = node;
    }

    queue->size++;
}

Node* pop(Queue *queue) {
    Node *temp = queue->front;

    queue->front = queue->front->next;

    return temp;
}


void init_scheduler(int sched_type) {
    logger = fopen("log.txt", "w");

    fprintf(logger, "[START init_scheduler]\n");
    fprintf(logger, "[Type=%d]\n", sched_type);
    ReadyQueue = malloc(sizeof(Queue));
    ReadyQueue->size = 0;
}

int scheduleme(float currentTime, int tid, int remainingTime, int tprio) {
    pthread_t callingThread = pthread_self();
    fprintf(logger, "[SCHEDULEME] [Called By=%u] ", callingThread);
    fprintf(logger, "currentTime=%f, tid=%d, remainingTime=%d, tprio=%d\n", currentTime, tid, remainingTime, tprio);

    CURRENT_TIME = currentTime;

    Node *newNode =  malloc(sizeof(Node));
    newNode->thread = malloc(sizeof(Thread));
    newNode->thread->id = tid;
    newNode->thread->arrival_time = currentTime;
    newNode->thread->required_time = remainingTime;
    newNode->thread->priority = tprio;

    //push(ReadyQueue, newNode);

    return 0;
}