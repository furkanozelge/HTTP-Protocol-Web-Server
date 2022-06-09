#include <stdio.h>
#include "request.h"
#include "io_helper.h"
#include <pthread.h>

char default_root[] = ".";

//
// ./wserver [-p <portnum>] [-t threads] [-b buffers]
//
// e.g.
// ./wserver -p 2022 -t 5 -b 10
//
typedef struct node
{
    int val;
    struct node *next;
} node;

typedef struct queue_thread
{
    int size;
    node *head_node;
    node *end_node;
} queue;
// Create a new node

queue *buff;
pthread_cond_t empty_t, fill_t;
pthread_mutex_t mut;

node *newNode(int value)
{
    node *nd = (node *)malloc(sizeof(node));
    nd->val = value;
    nd->next = NULL;
    return nd;
}

// Create a new queue
queue *newQueue()
{
    queue *que = (queue *)malloc(sizeof(queue));
    que->size = 0;
    que->head_node = NULL;
    que->end_node = NULL;
    return que;
}
// Find Size
int Size(queue *que)
{
    return que->size;
}
//Adding 
void enqueue(queue *que, int value)
{
   
    node *que2;
    if (Size(que) == 0)
    {
        que2 = newNode(value);
        que->head_node = que2;
        que->end_node = que2;
        que->size++;
        return;
    }
    // if it is not empty
    que2 = newNode(value);
    que->end_node->next = que2;
    que->end_node = que2;
    que->size++;
    return;
}

// remove/delete from a queue
int dequeue(queue *que)
{
    if (que->head_node == NULL)
    {
        return -1;
    }
    int que2 = que->head_node->val;
    que->size--;
    que->head_node = que->head_node->next;
    return que2;
}

// Create Worker Thread
void *worker_t()
{
    int conn_fd;
    pthread_mutex_lock(&mut);
    while (Size(buff) == 0)
    {
        pthread_cond_wait(&fill_t, &mut);
    }
    conn_fd = dequeue(buff);
    pthread_cond_signal(&empty_t);
    pthread_mutex_unlock(&mut);
    printf("thread %ud is working on fd %d and buffSize is %d\n", pthread_self(), conn_fd, Size(buff));
    request_handle(conn_fd); // Request is handled
    close_or_die(conn_fd);
}

int main(int argc, char *argv[])
{
    int c;
    char *root_dir = default_root;
    int port = 10000;
    int threads = 2;
    int buffer_size = 5;
    buff = newQueue();

    while ((c = getopt(argc, argv, "p:t:b:")) != -1)
        switch (c)
        {
        case 'p':
            port = atoi(optarg);
            break;
        case 't':
            threads = atoi(optarg);
            break;
        case 'b':
            buffer_size = atoi(optarg);
            break;
        default:
            fprintf(stderr, "usage: ./wserver [-p <portnum>] [-t threads] [-b buffers] \n");
            exit(1);
        }

    printf("Server running on port: %d, threads: %d, buffer: %d\n", port, threads, buffer_size);

    // run out of this directory
    chdir_or_die(root_dir);
    pthread_t *worker_th = (pthread_t *)malloc(buffer_size * sizeof(pthread_t));

    for (int i = 0; i < buffer_size; i++)
    {
        pthread_create(&worker_th[i], NULL, worker_t, NULL);
    }

    for (int i = 0; i < buffer_size; i++)
    {

        sleep(2);
    }

    // now, get to work
    int listen_fd = open_listen_fd_or_die(port);
    while (1)
    {
        struct sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        int conn_fd = accept_or_die(listen_fd, (sockaddr_t *)&client_addr, (socklen_t *)&client_len);
        pthread_mutex_lock(&mut);
        while (Size(buff) == buffer_size)
        {
            pthread_cond_wait(&empty_t, &mut);
        }
        enqueue(buff, conn_fd); //Adding queue
        pthread_cond_signal(&fill_t);
        pthread_mutex_unlock(&mut);
    }
    return 0;
}
