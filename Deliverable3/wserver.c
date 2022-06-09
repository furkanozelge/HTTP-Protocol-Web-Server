#include <stdio.h>
#include <pthread.h>
#include "request.h"
#include "io_helper.h"

char default_root[] = ".";

//
// ./wserver [-p <portnum>] [-t threads] [-b buffers]
//
// e.g.
// ./wserver -p 2022 -t 5 -b 10
//
typedef struct thread_pool pool_t;

struct thread_pool { //Define thread pool
    int *buffer;
    pthread_t *pool;
};

int buff = -1;
pthread_mutex_t mut;
pthread_cond_t full_t;
pthread_cond_t empty_t;



void *request_t(void *arg) { //Request handle 
    pool_t *threadP = (pool_t *)arg;
    int conn_fd;

    pthread_mutex_lock(&mut);
    while(buff == -1)
        pthread_cond_wait(&full_t,&mut);

    conn_fd = threadP->buffer[buff];
    buff--;
    pthread_cond_signal(&empty_t);
    pthread_mutex_unlock(&mut);

    request_handle(conn_fd);
	close_or_die(conn_fd);

    return (void*)0;
}

int main(int argc, char *argv[]) {
    int c;
    char *root_dir = default_root;
    int port = 10000;
	int threads = 2;
	int buffer_size = 5;

    while ((c = getopt(argc, argv, "p:t:b:")) != -1)
		switch (c) {
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
    // now, get to work
    int listen_fd = open_listen_fd_or_die(port);

    pool_t *threadP = (pool_t *)malloc(sizeof(pool_t));
    pthread_mutex_init(&mut, NULL);
    pthread_cond_init(&full_t, NULL);
    pthread_cond_init(&empty_t, NULL);
    threadP->buffer = (int *)malloc(buffer_size*sizeof(int));
    threadP->pool = (pthread_t *)malloc(threads*sizeof(pthread_t));
    for(int i = 0; i < threads; ++i) {
        pthread_create(&threadP->pool[i], NULL, request_t, (void*)threadP);
    }

    while (1) {
	    struct sockaddr_in client_addr;
	    int client_len = sizeof(client_addr);
	    int conn_fd = accept_or_die(listen_fd, (sockaddr_t *) &client_addr, (socklen_t *) &client_len);
        pthread_mutex_lock(&mut);
        while (buff == buffer_size - 1)
            pthread_cond_wait(&empty_t, &mut);

        buff++;
        threadP->buffer[buff] = conn_fd;

        pthread_cond_signal(&full_t);
        pthread_mutex_unlock(&mut);
    }
    return 0;
}






