
#include <math.h>
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


// Prototypes
void push(Queue*, Node*);
Node* pop(Queue*);
int queue_contains_thread(Queue*, int);
Thread* queue_get_thread(Queue*, int);
void print_queue(Queue*);

Queue *ReadyQueue;
pthread_mutex_t queue_lock;
pthread_mutex_t executing_lock;
pthread_cond_t executing_cond;

void init_scheduler(int sched_type) {
    //logger = fopen("log.txt", "w");

    printf(" [START init_scheduler]\n");
    printf(" [Type=%d]\n", sched_type);

    pthread_cond_init(&executing_cond, NULL);

    ReadyQueue = malloc(sizeof(Queue));
    ReadyQueue->size = 0;
}

int scheduleme(float currentTime, int tid, int remainingTime, int tprio) {
    printf(" \t[SCHEDULEME] ");
    printf("currentTime=%f, tid=%d, remainingTime=%d, tprio=%d, FRONT Thread=%d\n", currentTime, tid, remainingTime, tprio, (ReadyQueue->front != NULL ? ReadyQueue->front->thread->id : -1));

    // // Add thread to the ready queue if it isn't already in there.
    if (queue_contains_thread(ReadyQueue, tid) == 0) {

        Node *newNode =  malloc(sizeof(Node));
        newNode->thread = malloc(sizeof(Thread));
        newNode->thread->id = tid;
        newNode->thread->arrival_time = currentTime;
        newNode->thread->required_time = remainingTime;
        newNode->thread->priority = tprio;

        // Lock the queue, so multiple threads aren't trying to add to it at the same time.
        pthread_mutex_lock(&queue_lock);
        push(ReadyQueue, newNode);
        pthread_mutex_unlock(&queue_lock);
    } else {

    }

    // Block current thread as long as it's not at the front of the queue
    pthread_mutex_lock(&executing_lock);
    while (ReadyQueue->front->thread->id != tid) {
        printf("\t[BLOCK THREAD] tid=%d\n", tid);
        pthread_cond_wait(&executing_cond, &executing_lock);
    }
    pthread_mutex_unlock(&executing_lock);

    printf("\t[EXECUTING THREAD] tid=%d\n", tid);

    Thread *currentThread = queue_get_thread(ReadyQueue, tid);
    currentThread->required_time = remainingTime;

    // Once required time = 0, thread is finished executing. Pop the front of the queue,
    // and signal all threads to resume executing. (Each thread goes back to while loop
    // and checks if they're at the front he queue again)
    if (currentThread->required_time == 0) {
        // Only 1 thread should be executing here at all times, so no need to lock the queue.
        pop(ReadyQueue);
        pthread_mutex_lock(&executing_lock);
        pthread_cond_signal(&executing_cond);
        pthread_mutex_unlock(&executing_lock);
    }

    return ceil(currentTime);
}

// Adds a node to the end of the queue.
void push(Queue *queue, Node *node) {
    printf("\t\t[ADDED TO READY QUEUE] [size=%d]", queue->size + 1);
    printf(" tid=%d arrival_time=%f required_time=%d priorty=%d\n",
        node->thread->id, node->thread->arrival_time, node->thread->required_time, node->thread->priority);

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
Node* pop(Queue *queue) {
    printf("\t\t[POP QUEUE] size=%d, front=%d, next=%d\n", queue->size - 1, queue->front->thread->id, (queue->front->next != NULL ? queue->front->next->thread->id : -1));

    Node *temp = queue->front;

    if (queue->front->next != NULL) {
        queue->front = queue->front->next;
        printf("\t\t\t[New front] tid=%d\n", queue->front->thread->id);
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
    printf("\t\t\t[CURRENT QUEUE] ");
    Node* current = queue->front;
    while (current != NULL) {
        printf("[%d] ", current->thread->id);
        current = current->next;
    }
    printf("\n");
}