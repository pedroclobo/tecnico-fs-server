#include "operations.h"
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define S 1
#define PIPE_NAME_MAX_SIZE 41

typedef struct {
	pthread_t thread;
	bool free;
	int pipe;
} session;

session sessions[S];

int main(int argc, char **argv) {

	if (argc < 2) {
		printf("Please specify the pathname of the server's pipe.\n");
		return 1;
	}
	char *pipename = argv[1];
	printf("Starting TecnicoFS server with pipe called %s\n", pipename);

	/* Unlink and create server pipe */
	if (unlink(pipename) != 0 && errno != ENOENT) {
		return -1;
	}
	if (mkfifo(pipename, 0640) < 0) {
		return -1;
	}

	/* Open server pipe */
	int server_pipe;
	if ((server_pipe = open(pipename, O_RDONLY)) != 0) {
		exit(-1);
	}

	/* Read op-code */
	int op_code;
	read(server_pipe, &op_code, sizeof(int));

	if (op_code == TFS_OP_CODE_MOUNT) {
		int session_id;
		for (int s = 0; s < S; s++) {
			if (sessions[s].free) {
				session_id = s;
			}
		}
		sessions[session_id].free = false;

		char client_pipe_path[PIPE_NAME_MAX_SIZE];
		ssize_t ret;
		ret = read(server_pipe, client_pipe_path, sizeof(client_pipe_path));
		if (ret < 0) {
			return -1;
		}

		if ((sessions[session_id].pipe = open(client_pipe_path, O_WRONLY)) != 0) {
			return -1;
		}

		ssize_t written;
		while (written < 1) {
			ret = write(sessions[session_id].pipe, &session_id, 1);
			written += ret;
			if (ret < 0) {
				return -1;
			}
		}

	} else if (op_code == TFS_OP_CODE_UNMOUNT) {
		int session_id;
		read(server_pipe, &session_id, sizeof(int));
		sessions[session_id].free = true;
	}

	/* Close and unlink server pipe */
	close(server_pipe);
	unlink(pipename);

	return 0;
}
