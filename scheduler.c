#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

struct _thread {
    int id;
    float arrival_time;
    int required_time;
    int priority;
    pthread_mutex_t execute_mutex;
    pthread_cond_t execute_cond;
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
void push(Queue*, Node*);
Node* pop(Queue*);
int queue_contains_thread(Queue*, int);
Thread* queue_get_thread(Queue*, int);


Queue *ReadyQueue;
pthread_mutex_t queue_lock;

void init_scheduler(int sched_type) {
    //logger = fopen("log.txt", "w");

    printf(" [START init_scheduler]\n");
    printf(" [Type=%d]\n", sched_type);

    ReadyQueue = malloc(sizeof(Queue));
    ReadyQueue->size = 0;
}

int scheduleme(float currentTime, int tid, int remainingTime, int tprio) {
    printf(" \t[SCHEDULEME] ");
    printf("currentTime=%f, tid=%d, remainingTime=%d, tprio=%d, FRONT Thread=%d\n", currentTime, tid, remainingTime, tprio, (ReadyQueue->front != NULL ? ReadyQueue->front->thread->id : -1));

    // Add thread to the ready queue if it isn't already in there.
    pthread_mutex_lock(&queue_lock);
    if (queue_contains_thread(ReadyQueue, tid) == 0) {

        Node *newNode =  malloc(sizeof(Node));
        newNode->thread = malloc(sizeof(Thread));
        newNode->thread->id = tid;
        newNode->thread->arrival_time = currentTime;
        newNode->thread->required_time = remainingTime;
        newNode->thread->priority = tprio;
        pthread_mutex_init(&newNode->thread->execute_mutex, NULL);
        pthread_cond_init(&newNode->thread->execute_cond, NULL);

        push(ReadyQueue, newNode);
    }

    Thread *currentThread = queue_get_thread(ReadyQueue, tid);
    pthread_mutex_unlock(&queue_lock);

    // Block current thread if it's not at the front of the queue.
    pthread_mutex_lock(&currentThread->execute_mutex);
    while (ReadyQueue->front->thread->id != tid) {
        printf("waiting...\n");
        pthread_cond_wait(&currentThread->execute_cond, &currentThread->execute_mutex);
    }
    pthread_mutex_unlock(&currentThread->execute_mutex);

    printf("\t[EXECUTE THREAD] tid=%d\n", tid);

    // When remaining time hits 0, thread has completed and needs to be removed from the queue.
    // The next thread is then signaled to execute
    if (remainingTime == 0) {
        printf("\t[remainingTime=0]\n");
        pthread_mutex_lock(&queue_lock);
        pop(ReadyQueue);

        pthread_mutex_unlock(&queue_lock);

        // Signal the front of the queue to execute.
        if (ReadyQueue->front != NULL) {
            printf("\t[SIGNAL FRONT] tid=%d\n", ReadyQueue->front->thread->id);
            pthread_mutex_lock(&ReadyQueue->front->thread->execute_mutex);
            pthread_cond_signal(&ReadyQueue->front->thread->execute_cond);
            pthread_mutex_unlock(&ReadyQueue->front->thread->execute_mutex);
        }
    }

    return currentTime;
}

// Adds a node to the end of the queue.
void push(Queue *queue, Node *node) {
    printf("\t\t[ADDED TO READY QUEUE] [size=%d]", queue->size + 1);
    printf(" tid=%d arrival_time=%f required_time=%d priorty=%d\n",
        node->thread->id, node->thread->arrival_time, node->thread->required_time, node->thread->priority);

    if (queue->size == 0) {
        queue->front = node;
    } else {
        if (queue->back == NULL) {
            queue->front->next = node;
        } else {
            queue->back->next = node;
        }
    }

    queue->size++;
}

// Removes the first node in the queue and returns it
Node* pop(Queue *queue) {
    printf("\t\t[POP QUEUE] size=%d\n", queue->size - 1);
    Node *temp = queue->front;

    if (queue->front->next != NULL) {
        queue->front = queue->front->next;
        printf("\t\t\t[New front] tid=%d\n", queue->front->thread->id);
    } else {
        queue->front = NULL;
    }
    queue->size--;

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