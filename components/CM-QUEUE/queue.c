// C program for array implementation of queue
#include "queue.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"


// A structure to represent a queue

typedef  struct node {
        char msg[QUEUE_MAX_MSG_SIZE];
}lpp_node_t;

struct Queue
{
        int front, rear, size;
        unsigned capacity;
        lpp_node_t* array;
};

struct Queue* queue;

// function to create a queue of given capacity.
// It initializes size of queue as 0
struct Queue* createQueue(unsigned capacity)
{
        struct Queue* queue = (struct Queue*) malloc(sizeof(struct Queue));
        queue->capacity = capacity;
        queue->front = queue->size = 0;
        queue->rear = capacity - 1; // This is important, see the enqueue
        queue->array = (lpp_node_t*) malloc(queue->capacity * sizeof(lpp_node_t));
        return queue;
}

// Queue is full when size becomes equal to the capacity
int isFull()
{
        return (queue->size == queue->capacity);
}

// Queue is empty when size is 0
int isEmpty()
{
        return (queue->size == 0);
}



// Function to add an item to the queue.
// It changes rear and size
void enqueue(char *item)
{
        if (isFull(queue))
                return;

                queue->rear = (queue->rear + 1)%queue->capacity;
                //queue->array[queue->rear] = item;
                memset(queue->array[queue->rear].msg, 0, sizeof(queue->array[queue->rear].msg));
                strncpy(queue->array[queue->rear].msg, item, sizeof(queue->array[queue->rear].msg) - 1);


                queue->size = queue->size + 1;

                // printf("%s enqueued to queue\n", item);
                ESP_LOGI("MSGQUEUE", "%s Enqueued to queue",item);


}

// Function to remove an item from queue.
// It changes front and size
char* dequeue()
{
        if (isEmpty())
                return INT_MIN;
        char* item = queue->array[queue->front].msg;
        queue->front = (queue->front + 1)%queue->capacity;
        queue->size = queue->size - 1;
        return item;
}

// Function to get front of queue
char* front()
{
        if (isEmpty())
                return INT_MIN;
        return queue->array[queue->front].msg;
}

// Function to get rear of queue
char* rear()
{
        if (isEmpty())
                return INT_MIN;
        return queue->array[queue->rear].msg;
}

void print_queue()
{
        for(int q = 0; q < queue->size; q++) {
                printf("%d %s\n",q, queue->array[q].msg);
        }
}

// Driver program to test above functions./
void queue_init()
{
        queue = createQueue(32);
}
