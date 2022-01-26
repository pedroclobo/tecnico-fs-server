#!/bin/bash

# Run db tests
#for file in $(ls --ignore="*.c" --ignore="*.sh" --ignore="test-cs*"); do
#	./"$file"
#	echo "$file"
#done

# Run cs tests
./../fs/tfs_server "/tmp/server.pipe" &
for file in $(ls --ignore="*.c" --ignore="*.sh" --ignore="test-db*"); do
	./"$file" "/tmp/client.pipe" "/tmp/server.pipe" &
done
