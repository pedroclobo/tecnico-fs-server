#include "../client/tecnicofs_client_api.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/**
 * Open and close tfs file.
 */

int main(int argc, char **argv) {

	char *path = "/f1";
	int f;

	if (argc < 3) {
		printf("You must provide the following arguments: 'client_pipe_path "
			   "server_pipe_path'\n");
		return 1;
	}

	assert(tfs_mount(argv[1], argv[2]) == 0);

	f = tfs_open(path, TFS_O_CREAT);
	assert(f != -1);

	assert(tfs_close(f) != -1);

	assert(tfs_unmount() == 0);

	printf("\033[0;32mSuccessful test.\n\033[0m");

	return 0;
}
