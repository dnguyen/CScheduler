#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define MULTI_LEVEL_QUEUE_SIZE 5

const int DEBUG = 0;
const int TIME_QUANTUMS[MULTI_LEVEL_QUEUE_SIZE] = { 5, 10, 15, 20, 25 };

struct _thread {
    int id;
    float arrival_time;
    int required_time;
    int priority;
    pthread_cond_t executing_cond;
    int quantum;
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
Node* init_new_node(int, float, int, int);
int queue_contains_thread(Queue*, int);
int multi_level_queue_contains_thread(int);
int get_executing_level();
Thread* get_thread_from_mlfq(int);
Thread* queue_get_thread(Queue*, int);
void print_queue(Queue*);
void print_multi_level_queue();

int FCFS(float, int, int, int);
int SRTF(float, int, int, int);
int MLFQ(float, int, int, int);
int PBS(float, int, int, int);

// Global variables
Queue *ReadyQueue;
Queue **MULTI_LEVEL_QUEUE;

pthread_mutex_t queue_lock;
pthread_mutex_t executing_lock;
pthread_cond_t executing_cond;

float GLOBAL_TIME;
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

    if (SCHED_TYPE == 3) {
        init_multi_level_queue();
    }
}

int scheduleme(float currentTime, int tid, int remainingTime, int tprio) {
    GLOBAL_TIME = currentTime;

    if (DEBUG == 1) {
        pthread_t thread = pthread_self();
        printf(" \t[SCHEDULEME (%u)] ", (int)thread);
        printf("currentTime=%f, tid=%d, remainingTime=%d, tprio=%d, FRONT Thread=%d\n", currentTime, tid, remainingTime, tprio, (ReadyQueue->front != NULL ? ReadyQueue->front->thread->id : -1));
    }

    // Check the type of scheduler and continue with the method specified
    if (SCHED_TYPE == 0) return FCFS(currentTime, tid, remainingTime, tprio);
    if (SCHED_TYPE == 1) return SRTF(currentTime, tid, remainingTime, tprio);
    if (SCHED_TYPE == 2) return PBS(currentTime, tid, remainingTime, tprio);
    if (SCHED_TYPE == 3) return MLFQ(currentTime, tid, remainingTime, tprio);

}

// Implement the First Come First Serve Scheduler method
int FCFS(float currentTime, int tid, int remainingTime, int tprio) {
    pthread_mutex_lock(&queue_lock);

    if (ceil(currentTime) > GLOBAL_TIME) {
        GLOBAL_TIME = ceil(currentTime);
    }

    // Add thread to the ready queue if it isn't already in there.
    if (queue_contains_thread(ReadyQueue, tid) == 0) {
        Node *newNode = init_new_node(tid, currentTime, remainingTime, tprio);
        Enqueue(ReadyQueue, newNode);
    }

    // Block current thread as long as it's not at the front of the queue
    while (ReadyQueue->front->thread->id != tid) {
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

    return GLOBAL_TIME;

}

int SRTF(float currentTime, int tid, int remainingTime, int tprio) {

    pthread_mutex_lock(&queue_lock);

    // // Add thread to the ready queue if it isn't already in there.
    if (queue_contains_thread(ReadyQueue, tid) == 0) {
        Node *newNode = init_new_node(tid, currentTime, remainingTime, tprio);
        insertNodeSRTF(ReadyQueue, newNode);
    }

    ReadyQueue->front->thread->arrival_time = currentTime;

    // Find any threads that have the same priority as the current thread
    // If there are, check their arrival times, and move the one with the lower
    // arrival time closer to the front. Only need to do this check if the thread
    // being scheduled is at the front of the queue.
    Thread *schedulingThread = queue_get_thread(ReadyQueue, tid);
    if (tid == ReadyQueue->front->thread->id) {
        Node *current = ReadyQueue->front;
        int do_swap = 0;
        while (current != NULL && current->thread->required_time == remainingTime) {
            if (current->thread->arrival_time < currentTime) {
                do_swap = 1;
                break;
            }
            current = current->next;
        }

        // If a thread with equal priority and less arrival time was found swap it with the front thread.
        if (do_swap == 1) {
            ReadyQueue->front->next = current->next;
            current->next = ReadyQueue->front;
            ReadyQueue->front = current;
            pthread_cond_signal(&ReadyQueue->front->thread->executing_cond);
            print_queue(ReadyQueue);
        }
    }

    // Block current thread as long as it's not at the front of the queue
    while (ReadyQueue->front->thread->id != tid) {
        pthread_cond_wait(&schedulingThread->executing_cond, &queue_lock);
    }

    ReadyQueue->front->thread->required_time = remainingTime;

    // Once required time = 0, thread is finished executing. Pop the front of the queue,
    // and signal all threads to resume executing. (Each thread goes back to while loop
    // and checks if they're at the front of the queue again)
    if (ReadyQueue->front->thread->required_time == 0) {
        Dequeue(ReadyQueue);
        if (ReadyQueue->front != NULL) {
            pthread_cond_signal(&ReadyQueue->front->thread->executing_cond);
        }
    }

    pthread_mutex_unlock(&queue_lock);

    // GLOBAL_TIME should always be increasing with the currentTime
    if (ceil(currentTime) > GLOBAL_TIME) {
        GLOBAL_TIME = ceil(currentTime);
    }

    if (ReadyQueue->front != NULL) {
        pthread_cond_signal(&ReadyQueue->front->thread->executing_cond);
    }

    return GLOBAL_TIME;

}

int MLFQ(float currentTime, int tid, int remainingTime, int tprio) {
    pthread_mutex_lock(&queue_lock);

    // Add thread to the top-level queue if it isn't already contained in any of the queues
    if (multi_level_queue_contains_thread(tid) == 0) {
        Node *newNode = init_new_node(tid, currentTime, remainingTime, tprio);
        newNode->thread->quantum = 5;
        Enqueue(MULTI_LEVEL_QUEUE[0], newNode);
    }

    // Find the highest level queue that contains threads that need to be executed
    int current_level = get_executing_level();
    Thread *current_thread = get_thread_from_mlfq(tid);

    // Block all threads that are not at the head of the highest level queue
    while (MULTI_LEVEL_QUEUE[get_executing_level()]->front->thread->id != tid) {
        pthread_cond_wait(&current_thread->executing_cond, &queue_lock);
    }

    current_level = get_executing_level();

    // start executing thread
    MULTI_LEVEL_QUEUE[current_level]->front->thread->arrival_time = currentTime;
    MULTI_LEVEL_QUEUE[current_level]->front->thread->required_time = remainingTime;

    // When the executing threads remaining time is 0 dequeue it and signal the next thread to execute.
    // Else if the thread still has remaining time, but it has spent its quantum, then
    // move it to the next level.
    // If the thread can still execute, just decrement its quantum
    if (MULTI_LEVEL_QUEUE[current_level]->front->thread->required_time == 0) {
        Dequeue(MULTI_LEVEL_QUEUE[current_level]);
        if (MULTI_LEVEL_QUEUE[current_level]->front != NULL) {
            pthread_cond_signal(&MULTI_LEVEL_QUEUE[current_level]->front->thread->executing_cond);
        }
    } else if (MULTI_LEVEL_QUEUE[current_level]->front->thread->quantum == 0) {

        Node *newNode = init_new_node(
            MULTI_LEVEL_QUEUE[current_level]->front->thread->id,
            MULTI_LEVEL_QUEUE[current_level]->front->thread->arrival_time,
            MULTI_LEVEL_QUEUE[current_level]->front->thread->required_time,
            MULTI_LEVEL_QUEUE[current_level]->front->thread->priority);
        newNode->thread->executing_cond = MULTI_LEVEL_QUEUE[current_level]->front->thread->executing_cond;

        // Set threads quantum equal to the next level's quantum, unless we're on the last level. Then
        // set the threads quantum equal to the last level's quantum.
        if (current_level < MULTI_LEVEL_QUEUE_SIZE - 1) {
            newNode->thread->quantum = TIME_QUANTUMS[current_level + 1] - 1;

            // Do FCFS
            Enqueue(MULTI_LEVEL_QUEUE[current_level + 1], newNode);
            Dequeue(MULTI_LEVEL_QUEUE[current_level]);
        } else {
            newNode->thread->quantum = TIME_QUANTUMS[MULTI_LEVEL_QUEUE_SIZE - 1] - 1;
            // Do round robin for last level
            Enqueue(MULTI_LEVEL_QUEUE[current_level], newNode);
            Dequeue(MULTI_LEVEL_QUEUE[current_level]);
        }

        current_level = get_executing_level();

        // Front thread of the executing level has changed, so signal the new front thread to execute and
        // block the current thread.
        if (MULTI_LEVEL_QUEUE[current_level]->front->thread->id != tid) {
            pthread_cond_signal(&MULTI_LEVEL_QUEUE[current_level]->front->thread->executing_cond);
            pthread_cond_wait(&newNode->thread->executing_cond, &queue_lock);
        }
    } else {
        MULTI_LEVEL_QUEUE[current_level]->front->thread->quantum--;
    }

    // GLOBAL_TIME should always be increasing with the currentTime
    if (ceil(currentTime) > GLOBAL_TIME) {
        GLOBAL_TIME = ceil(currentTime);
    }

    current_level = get_executing_level();
    if (MULTI_LEVEL_QUEUE[current_level]->front != NULL) {
        pthread_cond_signal(&MULTI_LEVEL_QUEUE[current_level]->front->thread->executing_cond);
    }

    pthread_mutex_unlock(&queue_lock);

    return GLOBAL_TIME;
}


int PBS(float currentTime, int tid, int remainingTime, int tprio) {

    pthread_mutex_lock(&queue_lock);

    // Add thread to the ready queue if it isn't already in there.
    if (queue_contains_thread(ReadyQueue, tid) == 0) {
        Node *newNode = init_new_node(tid, currentTime, remainingTime, tprio);
        insertNodePBS(ReadyQueue, newNode);
    }

    // Update front nodes arrival time so we have the most up to date arrival time for the thread.
    ReadyQueue->front->thread->arrival_time = currentTime;

    // Find any threads that have the same priority as the current thread
    // If there are, check their arrival times, and move the one with the lower
    // arrival time closer to the front. Only need to do this check if the thread
    // being scheduled is at the front of the queue.
    Thread *schedulingThread = queue_get_thread(ReadyQueue, tid);
    if (tid == ReadyQueue->front->thread->id) {
        Node *current = ReadyQueue->front;
        int do_swap = 0;
        while (current != NULL && current->thread->priority == tprio) {
            if (current->thread->arrival_time < currentTime) {
                do_swap = 1;
                break;
            }
            current = current->next;
        }

        // If a thread with equal priority and less arrival time was found swap it with the front thread.
        if (do_swap == 1) {
            ReadyQueue->front->next = current->next;
            current->next = ReadyQueue->front;
            ReadyQueue->front = current;
            pthread_cond_signal(&ReadyQueue->front->thread->executing_cond);
            print_queue(ReadyQueue);
        }
    }

    // Block current thread as long as it's not at the front of the queue
    while (ReadyQueue->front->thread->id != tid) {
        pthread_cond_wait(&schedulingThread->executing_cond, &queue_lock);
    }

    ReadyQueue->front->thread->required_time = remainingTime;

    // Once required time = 0, thread is finished executing. Pop the front of the queue,
    // and signal all threads to resume executing. (Each thread goes back to while loop
    // and checks if they're at the front of the queue again)
    if (ReadyQueue->front->thread->required_time == 0) {
        Dequeue(ReadyQueue);
        if (ReadyQueue->front != NULL) {
            pthread_cond_signal(&ReadyQueue->front->thread->executing_cond);
        }
    }

    pthread_mutex_unlock(&queue_lock);

    // GLOBAL_TIME should always be increasing with the currentTime
    if (ceil(currentTime) > GLOBAL_TIME) {
        GLOBAL_TIME = ceil(currentTime);
    }

    if (ReadyQueue->front != NULL) {
        pthread_cond_signal(&ReadyQueue->front->thread->executing_cond);
    }

    return GLOBAL_TIME;

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

Node* init_new_node(int tid, float arrival_time, int required_time, int priority) {
    Node *newNode = malloc(sizeof(Node));
    newNode->thread = malloc(sizeof(Thread));
    newNode->thread->id = tid;
    newNode->thread->arrival_time = arrival_time;
    newNode->thread->required_time = required_time;
    newNode->thread->priority = priority;
    pthread_cond_init(&newNode->thread->executing_cond, NULL);

    return newNode;
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
        current = current->next;
    }
    return NULL;
}

void print_queue(Queue *queue) {
    if (DEBUG == 1) {
        printf("\t\t\t[CURRENT QUEUE] ");
        Node* current = queue->front;
        while (current != NULL) {
            printf("[%d {t=%d} {p=%d}] ", current->thread->id, (int)current->thread->arrival_time, current->thread->priority);
            current = current->next;
        }
        printf("\n");
    }
}

void print_multi_level_queue() {
    if (DEBUG == 1) {
        printf("\t\t\t[MULTI LEVEL QUEUE]\n");
        int i;
        for (i = 0; i < MULTI_LEVEL_QUEUE_SIZE; i++) {
             printf("\t\t\t[LEVEL = %d]\n\t", i);
            print_queue(MULTI_LEVEL_QUEUE[i]);
        }
    }
}

int get_executing_level() {
    int i = 0;
    Queue *currentExecutingLevel = MULTI_LEVEL_QUEUE[0];
    while (i < MULTI_LEVEL_QUEUE_SIZE) {
        if (MULTI_LEVEL_QUEUE[i]->front != NULL) {
            return i;
        }
        i++;
    }
}

Thread *get_thread_from_mlfq(int threadId) {
    Queue *currentQueue = MULTI_LEVEL_QUEUE[0];
    int i = 0;
    while (currentQueue != NULL) {
        Node *currentNode = currentQueue->front;

        while (currentNode != NULL) {
            if (currentNode->thread->id == threadId) {
                return currentNode->thread;
            }
            currentNode = currentNode->next;
        }

        i++;
        currentQueue = MULTI_LEVEL_QUEUE[i];
    }

    return NULL;
}
