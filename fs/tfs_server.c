#include "operations.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define S 100

int main(int argc, char **argv) {

	if (argc < 2) {
		printf("Please specify the pathname of the server's pipe.\n");
		return 1;
	}
	char *pipename = argv[1];
	printf("Starting TecnicoFS server with pipe called %s\n", pipename);

	/* Unlink and create server pipe */
	unlink(pipename);
	if (mkfifo(pipename, 0777) < 0) {
		exit(-1);
	}

	/* Open server pipe */
	int fserver;
	if ((fserver = open(pipename, O_RDONLY)) != 0) {
		exit(-1);
	};

	/* Close and unlink server pipe */
	close(fserver);
	unlink(pipename);

	return 0;
}
