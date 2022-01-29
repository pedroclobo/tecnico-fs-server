#include "operations.h"
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define S 1
#define PIPE_NAME_MAX_SIZE 41
#define BUFFER_SIZE 1000

typedef struct {
	char opcode;
	int session_id;
	int fhandle;
	int flags;
	size_t size;
	char *str;
} input_struct;

typedef struct {
	pthread_t thread;
	pthread_mutex_t lock_buffer;
	pthread_cond_t a;
	pthread_cond_t b;
	int count;
	char buffer[BUFFER_SIZE];
	bool free;
	int pipe;
} session;

session sessions[S];

void *slave_thread(void* arg) {
	int *session_id = (int*) arg;

	int sent_bytes = 0;
	session curr_session = sessions[session_id];

	size_t len = sizeof(char) + 3*sizeof(int) + sizeof(size_t) + size;

	while (sent_bytes < len) {
		if (pthread_mutex_lock(&curr_session.lock_buffer) == -1) {
			// mandar erro por pipe
		}

		while (curr_session.count == BUFFER_SIZE) {
			pthread_cond_wait(&curr_session.a, &curr_session.lock_buffer);
		}
		memcpy(curr_session.buffer + sent_bytes, &opcode, sizeof(char));
		sent_bytes += sizeof(int);

		if (fhandle != -1) {
			if (sizeof(int) + curr_session.count >= BUFFER_SIZE) {
				pthread_cond_wait(&curr_session.a, &curr_session.lock_buffer);
			}

			memcpy(curr_session.buffer + sent_bytes, &fhandle, sizeof(int));
			curr_session.count += sizeof(int);
		}

		if (flags != -1) {
			if (sizeof(int) + curr_session.count >= BUFFER_SIZE) {
				pthread_cond_wait(&curr_session.a, &curr_session.lock_buffer);
			}

			memcpy(curr_session.buffer + sent_bytes, &fhandle, sizeof(int));
			curr_session.count += sizeof(int);
		}

		if (size != -1) {
			if (sizeof(size_t) + curr_session.count >= BUFFER_SIZE) {
				pthread_cond_wait(&curr_session.a, &curr_session.lock_buffer);
			}

			memcpy(curr_session.buffer + sent_bytes, &size, sizeof(size_t));
			curr_session.count += sizeof(size_t);

			if (size + curr_session.count >= BUFFER_SIZE) {
				pthread_cond_wait(&curr_session.a, &curr_session.lock_buffer);
			}

			memcpy(curr_session.buffer + sent_bytes, buffer, size);
			curr_session.count += size;
		}

		pthread_cond_signal(&curr_session.b);

	/*resto*/

	return NULL;
}

int init_server() {
	for (int s = 0; s < S; s++) {
		if (pthread_create(&sessions[s].thread, NULL, slave_thread, &s) != 0) {
			return -1;
		}
		if (pthread_cond_init(&sessions[s].a, NULL) == -1) {
			return -1;
		}
		if (pthread_cond_init(&sessions[s].b, NULL) == -1) {
			return -1;
		}
		sessions[s].free = true;
	}

	if (tfs_init() == -1) {
		return -1;
	}

	return 0;
}

int main(int argc, char **argv) {

	/* Initialize server structures */
	init_server();

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

	while (1) {

		/* Parse input */
		char opcode;
		if (read(server_pipe, &opcode, sizeof(char)) == -1) {
			return -1;
		}

		int session_id;
		if (read(server_pipe, &session_id, sizeof(int)) == -1) {
			return -1;
		}

		int fhandle;
		if (read(server_pipe, &fhandle, sizeof(int)) == -1) {
			return -1;
		}

		int flags;
		if (read(server_pipe, &flags, sizeof(int)) == -1) {
			return -1;
		}

		size_t size;
		if (read(server_pipe, &size, sizeof(size_t)) == -1) {
			return -1;
		}

		/* TODO */
		char buffer[size];
		if (read(server_pipe, buffer, size) == -1) {
			return -1;
		}

		if (opcode == TFS_OP_CODE_MOUNT) {

			/* Compute new session_id */
			int session_id = -1;
			for (int s = 0; s < S; s++) {
				if (sessions[s].free == true) {
					session_id = s;
				}
			}

			/* Maximum active sessions have been reached */
			if (session_id == -1) {
				return -1;
			}

			/* Thread is now occupied */
			sessions[session_id].free = false;

			/* Read client pipe path from client */
			char client_pipe_path[PIPE_NAME_MAX_SIZE];
			ssize_t ret;
			if ((ret = read(server_pipe, client_pipe_path, PIPE_NAME_MAX_SIZE - 1)) == -1) {
				return -1;
			}
			client_pipe_path[ret] = '\0';

			/* Open client pipe */
			if ((sessions[session_id].pipe = open(client_pipe_path, O_WRONLY)) == -1) {
				return -1;
			}

			/* Write session_id to client */
			if (write(sessions[session_id].pipe, &session_id, sizeof(int)) == -1) {
				return -1;
			}

		} else if (opcode == TFS_OP_CODE_UNMOUNT) {
			int session_id;
			if (read(server_pipe, &session_id, sizeof(int)) == -1) {
				return -1;
			}
			sessions[session_id].free = true;

		} else {
			int sent_bytes = 0;
			session curr_session = sessions[session_id];

			size_t len = sizeof(char) + 3*sizeof(int) + sizeof(size_t) + size;

			while (sent_bytes < len) {
				if (pthread_mutex_lock(&curr_session.lock_buffer) == -1) {
					return -1;
				}

				while (curr_session.count == BUFFER_SIZE) {
					pthread_cond_wait(&curr_session.a, &curr_session.lock_buffer);
				}
				memcpy(curr_session.buffer + sent_bytes, &opcode, sizeof(char));
				sent_bytes += sizeof(int);

				if (fhandle != -1) {
					if (sizeof(int) + curr_session.count >= BUFFER_SIZE) {
						pthread_cond_wait(&curr_session.a, &curr_session.lock_buffer);
					}

					memcpy(curr_session.buffer + sent_bytes, &fhandle, sizeof(int));
					curr_session.count += sizeof(int);
				}

				if (flags != -1) {
					if (sizeof(int) + curr_session.count >= BUFFER_SIZE) {
						pthread_cond_wait(&curr_session.a, &curr_session.lock_buffer);
					}

					memcpy(curr_session.buffer + sent_bytes, &fhandle, sizeof(int));
					curr_session.count += sizeof(int);
				}

				if (size != -1) {
					if (sizeof(size_t) + curr_session.count >= BUFFER_SIZE) {
						pthread_cond_wait(&curr_session.a, &curr_session.lock_buffer);
					}

					memcpy(curr_session.buffer + sent_bytes, &size, sizeof(size_t));
					curr_session.count += sizeof(size_t);

					if (size + curr_session.count >= BUFFER_SIZE) {
						pthread_cond_wait(&curr_session.a, &curr_session.lock_buffer);
					}

					memcpy(curr_session.buffer + sent_bytes, buffer, size);
					curr_session.count += size;
				}

				pthread_cond_signal(&curr_session.b);
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
