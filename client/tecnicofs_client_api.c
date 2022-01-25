#include "tecnicofs_client_api.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

int session_id;
int client_pipe;
int server_pipe;
char const *client_pipe_path;

int tfs_mount(char const *client_pipe_path, char const *server_pipe_path) {

	if (unlink(client_pipe_path) != 0 && errno != ENOENT) {
		return -1;
	}
	if (mkfifo(client_pipe_path, 0640) != 0) {
		return -1;
	}

	if ((server_pipe = open(server_pipe_path, O_RDONLY)) != 0) {
		return -1;
	}

	size_t written = 0;
	ssize_t ret;
	int op_code = TFS_OP_CODE_MOUNT;

	while (written < 1) {
		ret = write(server_pipe, &op_code, 1);
		written += ret;
		if (ret < 0) {
			return -1;
		}
	}

	written = 0;
	while (written < strlen(client_pipe_path)) {
		ret = write(server_pipe, client_pipe_path, strlen(client_pipe_path));
		written += ret;
		if (ret < 0) {
			return -1;
		}
	}

	if ((client_pipe = open(client_pipe_path, O_WRONLY)) != 0) {
		return -1;
	}

	size_t received = 0;
	while (received < sizeof(int)) {
		ret = read(client_pipe, &session_id, sizeof(int));
		received += ret;
		if (ret < 0) {
			return -1;
		}
	}

	return 0;
}

int tfs_unmount() {
	size_t written = 0;
	ssize_t ret;
	int op_code = TFS_OP_CODE_UNMOUNT;

	while (written < 1) {
		ret = write(server_pipe, &op_code, 1);
		written += ret;
		if (ret < 0) {
			return -1;
		}
	}

	written = 0;
	while (written < sizeof(int)) {
		ret = write(server_pipe, &session_id, sizeof(int));
		written += ret;
		if (ret < 0) {
			return -1;
		}
	}

	size_t received = 0;
	while (received < 4) {
		ret = read(client_pipe, &session_id, sizeof(int));
		received += ret;
		if (ret < 0) {
			return -1;
		}
	}

	if (close(server_pipe) != 0) {
		return -1;
	}
	if (close(client_pipe) != 0) {
		return -1;
	}
	if (unlink(client_pipe_path) != 0) {
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
