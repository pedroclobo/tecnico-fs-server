CC = gcc
CFLAGS = -std=c11 -D_POSIX_C_SOURCE=200809L -pthread

INCLUDE_DIRS := fs common .
INCLUDES = $(addprefix -I, $(INCLUDE_DIRS))

SOURCES := $(wildcard fs/*.c client/*.c common/*.c)
HEADERS := $(wildcard fs/*.h client/*.h common/*.h)
OBJECTS := $(SOURCES:.c=.o)

TEST_FILES := $(wildcard tests/*.c)
TEST_OBJECTS := $(TEST_FILES:.c=.o)
TESTS := $(TEST_FILES:.c=)

all:: $(OBJECTS)

test-db%: test-db%.o fs/operations.o fs/state.o
	$(CC) $(CFLAGS) $^ -o $@

test-cs%: test-cs%.o client/tecnicofs_client_api.o
	$(CC) $(CFLAGS) $^ -o $@

tests:: $(TESTS) $(OBJECTS) $(TEST_OBJECTS) $(TESTS)
	@for test in $(TESTS); do \
		./$$test; \
	done

clean::
	rm -f $(OBJECTS) $(TESTS) $(TEST_OBJECTS) *.txt *.zip
