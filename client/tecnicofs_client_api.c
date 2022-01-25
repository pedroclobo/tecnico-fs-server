#include "tecnicofs_client_api.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int tfs_mount(char const *client_pipe_path, char const *server_pipe_path) {
    if (unlink(client_pipe_path) != 0) {
		return -1;
    }
    if (mkfifo(client_pipe_path, 0640) != 0) {
        return -1;
    }

	int client_pipe;
	int server_pipe;
	if ((client_pipe = open(client_pipe_path, O_WRONLY)) != 0) {
		return -1;
	}
	if ((server_pipe = open(client_pipe_path, O_RDONLY)) != 0) {
		return -1;
	}

	char str[43] = { TFS_OP_CODE_MOUNT, '|', '\0' };
	strcat(str, client_pipe_path);

	ssize_t ret;

    size_t len = strlen(str);
    size_t written = 0;
    while (written < len) {
        ret = write(server_pipe, str + written, len - written);
        if (ret < 0) {
			return -1;
        }
        written += ret;
    }

	ssize_t received;
	char session_id[100];
    while (received != 0) {
        ret = read(server_pipe, session_id + received, 1);
        if (ret < 0) {
			return -1;
        }
        written += ret;
    }
	if (ret < 0) {
		return -1;
	}

	return 0;
}

int tfs_unmount() {
	/* TODO: Implement this */
	return -1;
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
