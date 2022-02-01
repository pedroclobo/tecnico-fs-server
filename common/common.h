#include<stdlib.h>

#ifndef COMMON_H
#define COMMON_H

/* tfs_open flags */
enum {
	TFS_O_CREAT = 0b001,
	TFS_O_TRUNC = 0b010,
	TFS_O_APPEND = 0b100,
};

/* operation codes (for client-server requests) */
enum {
	TFS_OP_CODE_MOUNT = 1,
	TFS_OP_CODE_UNMOUNT = 2,
	TFS_OP_CODE_OPEN = 3,
	TFS_OP_CODE_CLOSE = 4,
	TFS_OP_CODE_WRITE = 5,
	TFS_OP_CODE_READ = 6,
	TFS_OP_CODE_SHUTDOWN_AFTER_ALL_CLOSED = 7
};

#define PIPE_PATH_MAX_SIZE 41
#define FILE_NAME_MAX_SIZE 41
#define FILE_MAX_SIZE 1024

/* operation wrapper (for client-server requests) */
typedef struct {
	char opcode;
	int session_id;
	char client_pipe_path[PIPE_PATH_MAX_SIZE];
	char name[FILE_NAME_MAX_SIZE];
	int fhandle;
	int flags;
	char buffer[FILE_MAX_SIZE+1];
	size_t len;
} task_t;

#endif
