#include "operations.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Number of max session_ids */
#define S 20

/* Produtor-consumidor size */
#define BUFFER_SIZE 10

/* Indicate whether destroy has been called */
bool destroy_called = false;

/* Session_id structure */
typedef struct {
	pthread_t thread;
	pthread_mutex_t lock;
	pthread_cond_t can_consume;
	pthread_cond_t can_produce;
	task_t buffer[BUFFER_SIZE];
	int count;
	int consptr;
	int prodptr;
	bool free;
	int pipe;
} session;

session sessions[S];

int send_msg(int fildes, const void *buf, size_t nbytes, int session_id) {

	while (write(fildes, buf, nbytes) == -1) {

		/* Client pipe is no longer available */
		if (errno == EPIPE) {
			if (fildes != -1) {
				sessions[session_id].free = true;
			}
			return EPIPE;

		/* Process has been interrupted */
		} else if (errno == EINTR) {
			continue;
		} else {
			return -1;
		}
	}

	return 0;
}

void *thread_function(void *arg) {

	/* Grab thread session id */
	int session_id = *(int*) arg;

	/* Consumidor */
	while (true) {
		if (pthread_mutex_lock(&sessions[session_id].lock) == -1) {
			exit(EXIT_FAILURE);
		}
		while (sessions[session_id].count == 0) {
			if (pthread_cond_wait(&sessions[session_id].can_consume, &sessions[session_id].lock) != 0) {
				exit(EXIT_FAILURE);
			}
		}

		task_t operation = sessions[session_id].buffer[sessions[session_id].consptr];
		sessions[session_id].consptr++; if (sessions[session_id].consptr == BUFFER_SIZE) sessions[session_id].consptr = 0;
		sessions[session_id].count--;

		if (pthread_cond_signal(&sessions[session_id].can_produce) != 0) {
			exit(EXIT_FAILURE);
		}

		if (pthread_mutex_unlock(&sessions[session_id].lock) == -1) {
			exit(EXIT_FAILURE);
		}

		/* Treat request */
		switch (operation.opcode) {
		case TFS_OP_CODE_OPEN: {
			int ret = tfs_open(operation.name, operation.flags);
			send_msg(sessions[session_id].pipe, &ret, sizeof(int), session_id);
			break;
		}

		case TFS_OP_CODE_CLOSE: {
			int ret = tfs_close(operation.fhandle);
			send_msg(sessions[session_id].pipe, &ret, sizeof(int), session_id);
			break;
		}

		case TFS_OP_CODE_WRITE: {
			ssize_t ret = tfs_write(operation.fhandle, operation.buffer, operation.len);
			send_msg(sessions[session_id].pipe, &ret, sizeof(ssize_t), session_id);
			break;
		}

		case TFS_OP_CODE_READ: {
			ssize_t ret = tfs_read(operation.fhandle, operation.buffer, operation.len);
			if (send_msg(sessions[session_id].pipe, &ret, sizeof(ssize_t), session_id) != 0 || ret == -1) {
				break;
			}
			send_msg(sessions[session_id].pipe, operation.buffer, ret, session_id);
		}
		}
	}

	return NULL;
}

int init_server() {

	/* Prevent data races on threads initialization */
	int ids[S];
	for (int s = 0; s < S; s++) {
		ids[s] = s;
	}

	/* Initialize threads */
	for (int s = 0; s < S; s++) {
		if (pthread_mutex_init(&sessions[s].lock, NULL) == -1) {
			return -1;
		}
		if (pthread_cond_init(&sessions[s].can_consume, NULL) == -1) {
			return -1;
		}
		if (pthread_cond_init(&sessions[s].can_produce, NULL) == -1) {
			return -1;
		}
		sessions[s].count = 0;
		sessions[s].consptr = 0;
		sessions[s].prodptr = 0;

		sessions[s].free = true;

		/* Create slave threads */
		if (pthread_create(&sessions[s].thread, NULL, thread_function, &ids[s]) != 0) {
			return -1;
		}
	}

	/* Initialize tfs */
	if (tfs_init() == -1) {
		return -1;
	}

	return 0;
}

int main(int argc, char **argv) {

	/* Ignore sigpipe */
	signal(SIGPIPE, SIG_IGN);

	/* Initialize server */
	if (init_server() == -1) {
		exit(EXIT_FAILURE);
	}

	/* Get server pipe name from command line */
	if (argc < 2) {
		printf("Please specify the pathname of the server's pipe.\n");
		exit(EXIT_FAILURE);
	}
	char *pipename = argv[1];
	printf("Starting TecnicoFS server with pipe called %s\n", pipename);

	/* Unlink and create server pipe */
	if (unlink(pipename) == -1 && errno != ENOENT) {
		exit(EXIT_FAILURE);
	}
	if (mkfifo(pipename, 0777) == -1) {
		exit(EXIT_FAILURE);
	}

	/* Open server pipe */
	int server_pipe;
	if ((server_pipe = open(pipename, O_RDONLY)) == -1) {
		exit(EXIT_FAILURE);
	}

	/* Read and treat requests */
	while (true) {

		/* Read request */
		task_t operation;
		size_t r;
		while ((r = read(server_pipe, &operation, sizeof(task_t))) != sizeof(task_t)) {
			if (r == -1 && errno == EPIPE) {
				exit(EXIT_FAILURE);
			}
		}

		int ret = 0;
		switch (operation.opcode) {

		case TFS_OP_CODE_MOUNT: {

			/* Compute new session_id */
			int session_id = -1;
			for (int s = 0; s < S; s++) {
				if (sessions[s].free) {
					session_id = s;
					break;
				}
			}

			/* Open client pipe */
			int client_pipe = open(operation.client_pipe_path, O_WRONLY);
			if (client_pipe == -1) {
				break;
			}

			/* Maximum active sessions have been reached */
			if (session_id == -1) {
				ret = -1;
				send_msg(client_pipe, &ret, sizeof(int), -1);
				break;
			}

			/* Thread is now occupied */
			sessions[session_id].free = false;

			/* Assign client pipe */
			sessions[session_id].pipe = client_pipe;

			/* Write session_id to client */
			if (send_msg(sessions[session_id].pipe, &session_id, sizeof(int), session_id) != 0) {
				break;
			}

			/* Write return to client */
			send_msg(sessions[session_id].pipe, &ret, sizeof(int), session_id);

			break;
		}

		case TFS_OP_CODE_UNMOUNT: {

			/* Thread is now free */
			sessions[operation.session_id].free = true;

			/* Send return to client */
			send_msg(sessions[operation.session_id].pipe, &ret, sizeof(int), operation.session_id);

			break;
		}

		case TFS_OP_CODE_SHUTDOWN_AFTER_ALL_CLOSED: {

			int ret = tfs_destroy_after_all_closed();
			send_msg(sessions[operation.session_id].pipe, &ret, sizeof(int), operation.session_id);

			if (ret == 0) {
				destroy_called = true;
			}

			break;
		}

		case TFS_OP_CODE_OPEN: case TFS_OP_CODE_WRITE: case TFS_OP_CODE_READ: case TFS_OP_CODE_CLOSE: {

			if (pthread_mutex_lock(&sessions[operation.session_id].lock) == -1) {
				exit(EXIT_FAILURE);
			}

			while (sessions[operation.session_id].count == BUFFER_SIZE) {
				if (pthread_cond_wait(&sessions[operation.session_id].can_produce, &sessions[operation.session_id].lock) != 0) {
					exit(EXIT_FAILURE);
				}
			}

			sessions[operation.session_id].buffer[sessions[operation.session_id].prodptr] = operation;

			sessions[operation.session_id].prodptr++; if (sessions[operation.session_id].prodptr == BUFFER_SIZE) sessions[operation.session_id].prodptr = 0;
			sessions[operation.session_id].count++;

			if (pthread_cond_signal(&sessions[operation.session_id].can_consume) != 0) {
				exit(EXIT_FAILURE);
			}

			if (pthread_mutex_unlock(&sessions[operation.session_id].lock) == -1) {
				exit(EXIT_FAILURE);
			}

			break;
		}
		}

		/* Shutdown server */
		if (destroy_called) {
			break;
		}
	}

	/* Close and unlink server pipe */
	if (close(server_pipe) != 0) {
		exit(EXIT_FAILURE);
	}
	if (unlink(pipename) != 0) {
		exit(EXIT_FAILURE);
	}

	return 0;
}
