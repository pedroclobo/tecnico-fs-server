#include "tecnicofs_client_api.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

int session_id;
int client_pipe;
int server_pipe;
char client_pipe_file[PIPE_PATH_MAX_SIZE];

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

	/* Send request to server */
	task_t operation;
	operation.opcode = TFS_OP_CODE_MOUNT;
	strcpy(operation.client_pipe_path, client_pipe_path);
	if (write(server_pipe, &operation, sizeof(task_t)) == -1) {
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
	/* When no sessions are available, return comes earlier */
	if (session_id == -1) {
		return -1;
	}

	/* Receive return from server */
	int ret;
	if (read(client_pipe, &ret, sizeof(int)) == -1) {
		return -1;
	}

	return ret;
}

int tfs_unmount() {

	/* Send request to server */
	task_t operation;
	operation.opcode = TFS_OP_CODE_UNMOUNT;
	operation.session_id = session_id;
	if (write(server_pipe, &operation, sizeof(task_t)) == -1) {
		return -1;
	}

	/* Receive return from server */
	int ret;
	if (read(client_pipe, &ret, sizeof(int)) == -1) {
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

	return ret;
}

int tfs_open(char const *name, int flags) {

	/* Build request */
	task_t operation;
	operation.opcode = TFS_OP_CODE_OPEN;
	operation.session_id = session_id;
	strcpy(operation.name, name);
	operation.flags = flags;

	/* Send request to server */
	if (write(server_pipe, &operation, sizeof(task_t)) == -1) {
		return -1;
	}

	/* Receive return from server */
	int ret;
	if (read(client_pipe, &ret, sizeof(int)) == -1) {
		return -1;
	}

	return ret;
}

int tfs_close(int fhandle) {

	/* Build request */
	task_t operation;
	operation.opcode = TFS_OP_CODE_CLOSE;
	operation.session_id = session_id;
	operation.fhandle = fhandle;

	/* Send request to server */
	if (write(server_pipe, &operation, sizeof(task_t)) == -1) {
		return -1;
	}

	/* Receive return from server */
	int ret;
	if (read(client_pipe, &ret, sizeof(int)) == -1) {
		return -1;
	}

	return ret;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t len) {

	/* Build requeset */
	task_t operation;
	operation.opcode = TFS_OP_CODE_WRITE;
	operation.session_id = session_id;
	operation.fhandle = fhandle;
	operation.len = len;
	memcpy(operation.buffer, buffer, len);
	operation.buffer[len] = '\0';

	/* Send request to server */
	if (write(server_pipe, &operation, sizeof(task_t)) == -1) {
		return -1;
	}

	/* Receive return from server */
	ssize_t ret;
	if (read(client_pipe, &ret, sizeof(ssize_t)) == -1) {
		return -1;
	}

	return ret;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {

	/* Build request */
	task_t operation;
	operation.opcode = TFS_OP_CODE_READ;
	operation.session_id = session_id;
	operation.fhandle = fhandle;
	operation.len = len;

	/* Send request to server */
	if (write(server_pipe, &operation, sizeof(task_t)) == -1) {
		return -1;
	}

	/* Receive return from server */
	ssize_t size;
	if (read(client_pipe, &size, sizeof(ssize_t)) == -1) {
		return -1;
	}
	/* If read return an error, the buffer is not returned */
	if (size == -1) {
		return -1;
	}

	if (read(client_pipe, buffer, size) == -1) {
		return -1;
	}

	return size;
}

int tfs_shutdown_after_all_closed() {

	/* Build request */
	task_t operation;
	operation.opcode = TFS_OP_CODE_SHUTDOWN_AFTER_ALL_CLOSED;
	operation.session_id = session_id;

	/* Send request to server */
	if (write(server_pipe, &operation, sizeof(task_t)) == -1) {
		return -1;
	}

	/* Receive return from server */
	int ret;
	if (read(client_pipe, &ret, sizeof(int)) == -1) {
		return -1;
	}

	return 0;
}
