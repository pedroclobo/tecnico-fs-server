#include "operations.h"
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#define S 1
#define PIPE_NAME_MAX_SIZE 41

typedef struct {
	pthread_t thread;
	bool free;
	int pipe;
} session;

session sessions[S];

void init_server() {
	for (int s = 0; s < S; s++) {
		sessions[s].free = true;
	}
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

		/* Read op-code */
		char opcode;
		if (read(server_pipe, &opcode, sizeof(char)) == -1) {
			return -1;
		}

		if (opcode == TFS_OP_CODE_MOUNT) {

			/* Compute new session_id */
			int session_id = -1;
			for (int s = 0; s < S; s++) {
				if (sessions[s].free) {
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
			if (read(server_pipe, &session_id, sizeof(int) == -1)) {
				return -1;
			}
			sessions[session_id].free = true;
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
