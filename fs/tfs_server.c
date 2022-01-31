#include "operations.h"
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define S 20
#define BUFFER_SIZE 10

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

void print_operation(task_t *op) {
	printf("opcode: %d\n", op->opcode);
	printf("session_id: %d\n", op->session_id);
	printf("client_pipe_path: %s\n", op->client_pipe_path);
	printf("name: %s\n", op->name);
	printf("fhandle: %d\n", op->fhandle);
	printf("flags: %d\n", op->flags);
	printf("buffer: %s\n", op->buffer);
	printf("len: %lu\n", op->len);
}

void *thread_function(void *arg) {
	int session_id = *(int*) arg;

	while (true) {
		if (pthread_mutex_lock(&sessions[session_id].lock) == -1) {
			//
		}
		while (sessions[session_id].count == 0) {
			pthread_cond_wait(&sessions[session_id].can_consume, &sessions[session_id].lock);
		}

		task_t operation = sessions[session_id].buffer[sessions[session_id].consptr];
		sessions[session_id].consptr++; if (sessions[session_id].consptr == BUFFER_SIZE) sessions[session_id].consptr = 0;
		sessions[session_id].count--;

		pthread_cond_signal(&sessions[session_id].can_produce);

		if (pthread_mutex_unlock(&sessions[session_id].lock) == -1) {
			//
		}

		if (operation.opcode == TFS_OP_CODE_OPEN) {
			int ret = tfs_open(operation.name, operation.flags);
			// TODO error verification
			write(sessions[session_id].pipe, &ret, sizeof(int));

		} else if (operation.opcode == TFS_OP_CODE_CLOSE) {
			int ret = tfs_close(operation.fhandle);
			// TODO error verification
			write(sessions[session_id].pipe, &ret, sizeof(int));

		} else if (operation.opcode == TFS_OP_CODE_WRITE) {
			ssize_t ret = tfs_write(operation.fhandle, operation.buffer, operation.len);
			// TODO error verification
			write(sessions[session_id].pipe, &ret, sizeof(ssize_t));

		} else if (operation.opcode == TFS_OP_CODE_READ) {
			ssize_t ret = tfs_read(operation.fhandle, operation.buffer, operation.len);
			// TODO error verification
			write(sessions[session_id].pipe, &ret, sizeof(ssize_t));
			if (ret != -1) {
				write(sessions[session_id].pipe, operation.buffer, ret);
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

		/* Create slave thread */
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

	/* Initialize server */
	if (init_server() == -1) {
		return -1;
	}

	/* Get server pipe name from command line */
	if (argc < 2) {
		printf("Please specify the pathname of the server's pipe.\n");
		return 1;
	}
	char *pipename = argv[1];
	printf("Starting TecnicoFS server with pipe called %s\n", pipename);

	/* Unlink and create server pipe */
	if (unlink(pipename) == -1 && errno != ENOENT) {
		return -1;
	}
	if (mkfifo(pipename, 0777) == -1) {
		return -1;
	}

	/* Open server pipe */
	int server_pipe;
	if ((server_pipe = open(pipename, O_RDONLY)) == -1) {
		return -1;
	}

	while (true) {

		/* Read request */
		task_t operation;
		size_t r;
		while ((r = read(server_pipe, &operation, sizeof(task_t))) != sizeof(task_t)) {
			if (r == -1) {
				return -1;
			}
		}

		int ret = 0;

		if (operation.opcode == TFS_OP_CODE_MOUNT) {

			/* Compute new session_id */
			int session_id = -1;
			for (int s = 0; s < S; s++) {
				if (sessions[s].free) {
					session_id = s;
					break;
				}
			}

			/* Maximum active sessions have been reached */
			if (session_id == -1) {
				ret = -1;
			}

			/* Thread is now occupied */
			sessions[session_id].free = false;

			/* Open client pipe */
			if ((sessions[session_id].pipe = open(operation.client_pipe_path, O_WRONLY)) == -1) {
				ret = -1;
			}

			/* Write session_id to client */
			if (write(sessions[session_id].pipe, &session_id, sizeof(int)) == -1) {
				ret = -1;
			}

			// TODO error verification
			write(sessions[session_id].pipe, &ret, sizeof(int));

		} else if (operation.opcode == TFS_OP_CODE_UNMOUNT) {

			/* Thread is now free */
			sessions[operation.session_id].free = true;

			/* Send return to client */
			write(sessions[operation.session_id].pipe, &ret, sizeof(int));

		} else if (operation.opcode == TFS_OP_CODE_SHUTDOWN_AFTER_ALL_CLOSED) {

			int ret = tfs_destroy_after_all_closed();
			if (write(sessions[operation.session_id].pipe, &ret, sizeof(int)) == -1) {
				return -1;
			}

			if (ret == -1) {
				return -1;
			} else {
				break;
			}

		} else if (operation.opcode == TFS_OP_CODE_OPEN || operation.opcode == TFS_OP_CODE_WRITE || operation.opcode == TFS_OP_CODE_READ || operation.opcode == TFS_OP_CODE_CLOSE) {

			if (pthread_mutex_lock(&sessions[operation.session_id].lock) == -1) {
				//
			}

			while (sessions[operation.session_id].count == BUFFER_SIZE) {
				pthread_cond_wait(&sessions[operation.session_id].can_produce, &sessions[operation.session_id].lock);
			}

			sessions[operation.session_id].buffer[sessions[operation.session_id].prodptr] = operation;

			sessions[operation.session_id].prodptr++; if (sessions[operation.session_id].prodptr == BUFFER_SIZE) sessions[operation.session_id].prodptr = 0;
			sessions[operation.session_id].count++;

			pthread_cond_signal(&sessions[operation.session_id].can_consume);

			if (pthread_mutex_unlock(&sessions[operation.session_id].lock) == -1) {
				//
			}

		}
	}

	/* Close and unlink server pipe */
	if (close(server_pipe) != 0) {
		return -1;
	}
	if (unlink(pipename) != 0) {
		return -1;
	}

	return 0;
}
