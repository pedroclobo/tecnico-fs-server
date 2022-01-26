#include "tecnicofs_client_api.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#define PIPE_NAME_MAX_SIZE 41

int session_id;
int client_pipe;
int server_pipe;
char client_pipe_file[PIPE_NAME_MAX_SIZE];

int tfs_mount(char const *client_pipe_path, char const *server_pipe_path) {

	/* Create client pipe */
	if (unlink(client_pipe_path) == -1 && errno != ENOENT) {
		return -1;
	}
	if (mkfifo(client_pipe_path, 0640) == -1) {
		return -1;
	}
	strcpy(client_pipe_file, client_pipe_path);

	/* Open server pipe */
	if ((server_pipe = open(server_pipe_path, O_WRONLY)) == -1) {
		return -1;
	}

	/* Send opcode to server */
	char opcode = TFS_OP_CODE_MOUNT;
	if (write(server_pipe, &opcode, sizeof(char)) == -1) {
		return -1;
	}

	/* Send client pipe path to server */
	if (write(server_pipe, client_pipe_path, strlen(client_pipe_path)) == -1) {
		return -1;
	}

	/* Open client pipe */
	if ((client_pipe = open(client_pipe_path, O_RDONLY)) == -1) {
		return -1;
	}

	/* Receive session_id from server */
	if (read(client_pipe, &session_id, sizeof(int)) == -1) {
		return -1;
	}

	return 0;
}

int tfs_unmount() {

	/* Send opcode to server */
	char opcode = TFS_OP_CODE_UNMOUNT;
	if (write(server_pipe, &opcode, sizeof(char)) == -1) {
		return -1;
	}

	/* Send session_id to server */
	if (write(server_pipe, &session_id, sizeof(int)) == -1) {
		return -1;
	}

	/* Close server pipe */
	if (close(server_pipe) != 0) {
		return -1;
	}

	/* Close client pipe */
	if (close(client_pipe) != 0) {
		return -1;
	}

	/* Delete client pipe */
	if (unlink(client_pipe_file) != 0) {
		return -1;
	}

	return 0;
}

int tfs_open(char const *name, int flags) {
	/* TODO: Implement this */
	return -1;
}

int tfs_close(int fhandle) {
	/* TODO: Implement this */
	return -1;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t len) {
	/* TODO: Implement this */
	return -1;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
	/* TODO: Implement this */
	return -1;
}

int tfs_shutdown_after_all_closed() {
	/* TODO: Implement this */
	return -1;
}
