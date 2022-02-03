CC = gcc
CFLAGS = -std=c11 -D_POSIX_C_SOURCE=200809L -pthread

INCLUDE_DIRS := fs common .
INCLUDES = $(addprefix -I, $(INCLUDE_DIRS))

SOURCES := $(wildcard fs/*.c client/*.c common/*.c)
HEADERS := $(wildcard fs/*.h client/*.h common/*.h)
OBJECTS := $(SOURCES:.c=.o)
TARGET_EXECS := fs/tfs_server

TEST_FILES := $(wildcard tests/*.c)
TEST_OBJECTS := $(TEST_FILES:.c=.o)
TESTS := $(TEST_FILES:.c=)

all:: $(OBJECTS) $(TESTS) fs/tfs_server

test-db%: test-db%.o fs/operations.o fs/state.o
	$(CC) $(CFLAGS) $^ -o $@

test-cs%: test-cs%.o client/tecnicofs_client_api.o
	$(CC) $(CFLAGS) $^ -o $@

fs/tfs_server: fs/operations.o fs/state.o fs/tfs_server.o
	$(CC) $(CFLAGS) $^ -o $@

clean::
	rm -f $(OBJECTS) $(TESTS) $(TEST_OBJECTS) $(TARGET_EXECS) *.zip

zip::
	zip proj.zip Makefile fs/*.c fs/*.h common/*.h common/*.c client/*.c client/*.h
