#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

const int DEBUG = 0;

struct _thread {
    int id;
    float arrival_time;
    int original_required_time;
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

// Prototypes
void Enqueue(Queue*, Node*);
Node* Dequeue(Queue*);
void insertNodeSRTF(Queue*, Node*);
void insertNodePBS(Queue*, Node*);
int queue_contains_thread(Queue*, int);
int multi_level_queue_contains_thread(int);
Thread* queue_get_thread(Queue*, int);
void print_queue(Queue*);
void print_multi_level_queue();

int FCFS(float, int, int, int);
int SRTF(float, int, int, int);
int MLFQ(float, int, int, int);
int  PBS(float, int, int, int);


Queue *ReadyQueue;
#define MULTI_LEVEL_QUEUE_SIZE 5
const int TIME_QUANTUMS[MULTI_LEVEL_QUEUE_SIZE] = { 5, 10, 15, 20, 25 };
Queue **MULTI_LEVEL_QUEUE;

pthread_mutex_t queue_lock;
pthread_mutex_t executing_lock;
pthread_cond_t executing_cond;

// Keep track of 2 times: a global time, and the time schedule_me was called
// Should always be in sync with currentTime, but if currentTime, GLOBAL_TIME, SCHEDULE_ME_TIME
// ever go out of sync, we'll decide which one to use before returning from schedume_me
float GLOBAL_TIME;
float SCHEDULE_ME_TIME;

int SCHED_TYPE;

void init_multi_level_queue() {
    MULTI_LEVEL_QUEUE = malloc(MULTI_LEVEL_QUEUE_SIZE * sizeof(Queue));

    int i;
    for (i = 0; i < MULTI_LEVEL_QUEUE_SIZE; i++) {
        MULTI_LEVEL_QUEUE[i] = malloc(sizeof(Queue));
        MULTI_LEVEL_QUEUE[i]->size = 0;
    }
}

void init_scheduler(int sched_type) {
    SCHED_TYPE = sched_type;

    pthread_cond_init(&executing_cond, NULL);
    pthread_mutex_init(&queue_lock, NULL);

    ReadyQueue = malloc(sizeof(Queue));
    ReadyQueue->size = 0;

    init_multi_level_queue();
}

int scheduleme(float currentTime, int tid, int remainingTime, int tprio) {
    SCHEDULE_ME_TIME = currentTime;

    pthread_t thread = pthread_self();
    if (DEBUG == 1) {
        printf(" \t[SCHEDULEME (%u)] ", (int)thread);
        printf("currentTime=%f, tid=%d, remainingTime=%d, tprio=%d, FRONT Thread=%d\n", currentTime, tid, remainingTime, tprio, (ReadyQueue->front != NULL ? ReadyQueue->front->thread->id : -1));
    }

    // Check the type of scheduler and continue with the method specified
    if (SCHED_TYPE == 0) return FCFS(currentTime, tid, remainingTime, tprio);
    if (SCHED_TYPE == 1) return SRTF(currentTime, tid, remainingTime, tprio);
    if (SCHED_TYPE == 2) return  PBS(currentTime, tid, remainingTime, tprio);
    if (SCHED_TYPE == 3) return MLFQ(currentTime, tid, remainingTime, tprio);

}

// Implement the First Come First Serve Scheduler method
int FCFS(float currentTime, int tid, int remainingTime, int tprio) {
    pthread_mutex_lock(&queue_lock);

    if (currentTime > GLOBAL_TIME)
        GLOBAL_TIME = currentTime;

    // Add thread to the ready queue if it isn't already in there.
    if (queue_contains_thread(ReadyQueue, tid) == 0) {

        Node *newNode =  malloc(sizeof(Node));
        newNode->thread = malloc(sizeof(Thread));
        newNode->thread->id = tid;
        newNode->thread->arrival_time = currentTime;
        newNode->thread->required_time = remainingTime;
        newNode->thread->priority = tprio;

        // Lock the queue, so multiple threads aren't trying to add to it at the same time.
        Enqueue(ReadyQueue, newNode);
    }

    // Block current thread as long as it's not at the front of the queue
    while (ReadyQueue->front->thread->id != tid) {
        GLOBAL_TIME = currentTime;
        pthread_cond_wait(&executing_cond, &queue_lock);
    }

    ReadyQueue->front->thread->required_time = remainingTime;

    // Once required time = 0, thread is finished executing. Pop the front of the queue,
    // and signal all threads to resume executing. (Each thread goes back to while loop
    // and checks if they're at the front of the queue again)
    if (ReadyQueue->front->thread->required_time == 0) {
        Dequeue(ReadyQueue);
        pthread_cond_signal(&executing_cond);
    }

    pthread_mutex_unlock(&queue_lock);

    return (int)ceil(GLOBAL_TIME);

}

int SRTF(float currentTime, int tid, int remainingTime, int tprio) {

    pthread_mutex_lock(&queue_lock);

    // // Add thread to the ready queue if it isn't already in there.
    if (queue_contains_thread(ReadyQueue, tid) == 0) {
        Node *newNode = malloc(sizeof(Node));
        newNode->thread = malloc(sizeof(Thread));
        newNode->thread->id = tid;
        newNode->thread->arrival_time = ceil(currentTime);
        newNode->thread->required_time = remainingTime;
        newNode->thread->priority = tprio;

        // Lock the queue, so multiple threads aren't trying to add to it at the same time.
        insertNodeSRTF(ReadyQueue, newNode);
    }

    // Block current thread as long as it's not at the front of the queue
    while (ReadyQueue->front->thread->id != tid) {
        pthread_cond_wait(&executing_cond, &queue_lock);
    }

    ReadyQueue->front->thread->arrival_time = currentTime;
    ReadyQueue->front->thread->required_time = remainingTime;

    pthread_t thread = pthread_self();

    // Once required time = 0, thread is finished executing. Pop the front of the queue,
    // and signal all threads to resume executing. (Each thread goes back to while loop
    // and checks if they're at the front of the queue again)
    if (ReadyQueue->front->thread->required_time == 0) {
        Dequeue(ReadyQueue);
        if (ReadyQueue->front != NULL)
            pthread_cond_signal(&executing_cond);
    }

    pthread_mutex_unlock(&queue_lock);

    // GLOBAL_TIME should always be increasing with the currentTime
    if (ceil(currentTime) > SCHEDULE_ME_TIME) {
        SCHEDULE_ME_TIME = ceil(currentTime);
    }

    // when a thread resumes execution its currentTime was set to the time it was
    // first added to the queue, not the "real" currentTime.
    if (GLOBAL_TIME > SCHEDULE_ME_TIME) {
        SCHEDULE_ME_TIME = GLOBAL_TIME;
    }

    if (ReadyQueue->front != NULL)
        pthread_cond_signal(&executing_cond);

    return SCHEDULE_ME_TIME;

}

void insertNodeSRTF(Queue *queue, Node *node) {
    if (DEBUG == 1) {
    printf("\t\t[ADDED TO READY QUEUE] [size=%d]", queue->size + 1);
    printf(" tid=%d arrival_time=%f required_time=%d priorty=%d\n",
    node->thread->id, node->thread->arrival_time, node->thread->required_time, node->thread->priority);
    }
    if (queue->front == NULL || queue->front->thread->required_time > node->thread->required_time) {
        node->next = queue->front;
        queue->front = node;
    } else {
        // Locate the node before the point of insertion
        Node *current = queue->front;
        while (current->next != NULL && current->next->thread->required_time < node->thread->required_time){

            current = current->next;
        }

        // If the node we're inserting and the node we're inserting it after (current) have the same remaining time
        // then look at their arrival times. the one with the smaller arrival time will be placed first.
        if (current->thread->required_time == node->thread->required_time) {
            if (current->thread->arrival_time < node->thread->arrival_time) {
                node->next = current->next;
                current->next = node;
            } else {
                node->next = current;
                current->next = current->next->next;
            }
        } else {
            node->next = current->next;
            current->next = node;
        }
    }

    queue->size++;
    print_queue(queue);

}

int MLFQ(float currentTime, int tid, int remainingTime, int tprio) {
    pthread_mutex_lock(&queue_lock);
    GLOBAL_TIME = ceil(currentTime);
    pthread_mutex_unlock(&queue_lock);

        pthread_mutex_lock(&queue_lock);
    // Add thread to the top-level queue if it isn't already contained in any of the queues
    if (multi_level_queue_contains_thread(tid) == 0) {

        Node *newNode = malloc(sizeof(Node));
        newNode->thread = malloc(sizeof(Thread));
        newNode->thread->id = tid;
        newNode->thread->arrival_time = currentTime;
        newNode->thread->required_time = remainingTime;
        newNode->thread->original_required_time = remainingTime;
        newNode->thread->priority = tprio;

        // Lock the queue, so multiple threads aren't trying to add to it at the same time.
        //insertNodeSRTF(ReadyQueue, newNode);
        Enqueue(MULTI_LEVEL_QUEUE[0], newNode);
        print_multi_level_queue();
    }

    // Find the highest level queue that contains threads that need to be executed
    int i = 0;
    Queue *currentExecutingLevel = MULTI_LEVEL_QUEUE[0];
    while (i < MULTI_LEVEL_QUEUE_SIZE) {
        if (MULTI_LEVEL_QUEUE[i]->size > 0) {
            currentExecutingLevel = MULTI_LEVEL_QUEUE[i];
            break;
        }
        i++;
    }

        pthread_mutex_unlock(&queue_lock);
    printf("\t[HIGHEST LEVEL QUEUE] level=%d\n", i);

    // Block all threads that are not at the head of the highest level queue
    pthread_mutex_lock(&executing_lock);
    while (currentExecutingLevel->front->thread->id != tid) {
        pthread_mutex_lock(&queue_lock);
        GLOBAL_TIME = currentTime;
        pthread_mutex_unlock(&queue_lock);

        printf("\t[BLOCKING THREAD] tid=%d, currenttid=%d\n", tid, currentExecutingLevel->front->thread->id);
        pthread_cond_wait(&executing_cond, &executing_lock);

        if (currentExecutingLevel->size == 0) {
            printf("\t[CURRENT EXECUTING LEVEL SIZE = 0] i=%d\n", i);
            i++;
            currentExecutingLevel = MULTI_LEVEL_QUEUE[i];
        }
    }
    pthread_mutex_unlock(&executing_lock);
    // start executing thread
    printf("\t[EXECUTING THREAD] tid=%d, originalRemainingTime=%d, remainingTime=%d\n", tid, currentExecutingLevel->front->thread->original_required_time, remainingTime);
    //

    pthread_mutex_lock(&queue_lock);
    currentExecutingLevel->front->thread->required_time = remainingTime;
    pthread_mutex_unlock(&queue_lock);

    if (currentExecutingLevel->front->thread->required_time == 0) {
        pthread_mutex_lock(&executing_lock);
        Dequeue(currentExecutingLevel);
        pthread_cond_signal(&executing_cond);
        pthread_mutex_unlock(&executing_lock);
    } else if (currentExecutingLevel->front->thread->original_required_time - currentExecutingLevel->front->thread->required_time >= TIME_QUANTUMS[i]) {
        printf("\t\t[THREAD > TIME QUANTUM] quantum=%d\n", TIME_QUANTUMS[i]);
        printf("\t\t\t[PUSH %d from %d to %d]\n", currentExecutingLevel->front->thread->id, i, i + 1);
        pthread_mutex_lock(&executing_lock);
        Enqueue(MULTI_LEVEL_QUEUE[i + 1], Dequeue(MULTI_LEVEL_QUEUE[i]));

        //pthread_cond_wait(&executing_cond, &executing_lock);
        pthread_mutex_unlock(&executing_lock);
    }

    return ceil(GLOBAL_TIME);
}


int PBS(float currentTime, int tid, int remainingTime, int tprio) {

    pthread_mutex_lock(&queue_lock);

    // // Add thread to the ready queue if it isn't already in there.
    if (queue_contains_thread(ReadyQueue, tid) == 0) {
        Node *newNode = malloc(sizeof(Node));
        newNode->thread = malloc(sizeof(Thread));
        newNode->thread->id = tid;
        newNode->thread->arrival_time = ceil(currentTime);
        newNode->thread->required_time = remainingTime;
        newNode->thread->priority = tprio;

        // Lock the queue, so multiple threads aren't trying to add to it at the same time.
        insertNodePBS(ReadyQueue, newNode);
    }

    // Block current thread as long as it's not at the front of the queue
    while (ReadyQueue->front->thread->id != tid) {
        pthread_cond_wait(&executing_cond, &queue_lock);
    }

    ReadyQueue->front->thread->arrival_time = currentTime;
    ReadyQueue->front->thread->required_time = remainingTime;

    pthread_t thread = pthread_self();

    // Once required time = 0, thread is finished executing. Pop the front of the queue,
    // and signal all threads to resume executing. (Each thread goes back to while loop
    // and checks if they're at the front of the queue again)
    if (ReadyQueue->front->thread->required_time == 0) {
        Dequeue(ReadyQueue);
        if (ReadyQueue->front != NULL)
            pthread_cond_signal(&executing_cond);
    }

    pthread_mutex_unlock(&queue_lock);

    // GLOBAL_TIME should always be increasing with the currentTime
    if (ceil(currentTime) > SCHEDULE_ME_TIME) {
        SCHEDULE_ME_TIME = ceil(currentTime);
    }

    // when a thread resumes execution its currentTime was set to the time it was
    // first added to the queue, not the "real" currentTime.
    if (GLOBAL_TIME > SCHEDULE_ME_TIME) {
        SCHEDULE_ME_TIME = GLOBAL_TIME;
    }

    if (ReadyQueue->front != NULL)
        pthread_cond_signal(&executing_cond);

    return SCHEDULE_ME_TIME;

}

void insertNodePBS(Queue *queue, Node *node) {
    if (DEBUG == 1) {
    printf("\t\t[ADDED TO READY QUEUE] [size=%d]", queue->size + 1);
    printf(" tid=%d arrival_time=%f required_time=%d priorty=%d\n",
    node->thread->id, node->thread->arrival_time, node->thread->required_time, node->thread->priority);
    }

    // Compare the priority of the node we are inserting to the priority of the nodes that
    // are currently in the queue
    if (queue->front == NULL || queue->front->thread->priority > node->thread->priority) {
        node->next = queue->front;
        queue->front = node;
    } else {
        // Locate the node before the point of insertion
        Node *current = queue->front;
        while (current->next != NULL && current->next->thread->priority < node->thread->priority){

            current = current->next;
        }

        // If the node we're inserting and the node we're inserting it after (current) 
        // have the same priority, then look at their arrival times.
        // The one with the smaller arrival time will be placed first.
        if (current->thread->priority == node->thread->priority) {
            if (current->thread->arrival_time < node->thread->arrival_time) {
                node->next = current->next;
                current->next = node;
            } else {
                node->next = current;
                current->next = current->next->next;
            }
        } else {
            node->next = current->next;
            current->next = node;
        }
    }

    queue->size++;
    print_queue(queue);

}


// Adds a node to the end of the queue.
void Enqueue(Queue *queue, Node *node) {
    if (DEBUG == 1) {
    printf("\t\t[ADDED TO READY QUEUE] [size=%d]", queue->size + 1);
    printf(" tid=%d arrival_time=%f required_time=%d priorty=%d\n",
        node->thread->id, node->thread->arrival_time, node->thread->required_time, node->thread->priority);
    }
    node->next = NULL;
    if (queue->front == NULL) {
        queue->front = node;
    } else {
        Node *current = queue->front;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = node;
    }
    queue->size++;
    print_queue(queue);
}

// Removes the first node in the queue and returns it
Node* Dequeue(Queue *queue) {
    if (DEBUG == 1) {
    printf("\t\t[POP QUEUE] size=%d, front=%d, next=%d\n", queue->size - 1, queue->front->thread->id, (queue->front->next != NULL ? queue->front->next->thread->id : -1));
    }
    Node *temp = queue->front;

    if (queue->front->next != NULL) {
        queue->front = queue->front->next;
    } else {
        queue->front = NULL;
    }
    queue->size--;

    print_queue(queue);

    return temp;
}

// Checks if the queue contains the thread id
// 0 = false, 1 = true
int queue_contains_thread(Queue *queue, int threadId) {
    Node *current = queue->front;
    while (current != NULL) {
        if (current->thread->id == threadId) {
            return 1;
        }
        current = current->next;
    }

    return 0;
}

int multi_level_queue_contains_thread(int threadId) {
    int i;
    for (i = 0; i < MULTI_LEVEL_QUEUE_SIZE; i++) {
        if (queue_contains_thread(MULTI_LEVEL_QUEUE[i], threadId) == 1) {
            return 1;
        }
    }
    return 0;
}

// Gets a thread from the queue by thread id.
Thread *queue_get_thread(Queue *queue, int threadId) {
    Node *current = queue->front;
    while (current != NULL) {
        if (current->thread->id == threadId) {
            return current->thread;
        }
    }
    return NULL;
}

void print_queue(Queue *queue) {
    if (DEBUG == 1) {
    printf("\t\t\t[CURRENT QUEUE] ");
    Node* current = queue->front;
    while (current != NULL) {
        printf("[%d] ", current->thread->id);
        current = current->next;
    }
    printf("\n");
    }
}

void print_multi_level_queue() {
    printf("\t\t\t[MULTI LEVEL QUEUE]\n");
    int i;
    for (i = 0; i < MULTI_LEVEL_QUEUE_SIZE; i++) {
        printf("\t\t\t[LEVEL = %d]\n\t", i);
        print_queue(MULTI_LEVEL_QUEUE[i]);
    }
}
