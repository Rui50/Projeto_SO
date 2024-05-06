CC = gcc
CFLAGS = -Wall -g
LDFLAGS =

all: folders server client

server: bin/orchestrator

client: bin/client

folders:
	@mkdir -p src include obj bin tmp

bin/orchestrator: obj/orchestrator.o obj/task.o obj/taskQueue.o obj/requests.o obj/execute.o
	$(CC) $(LDFLAGS) $^ -o $@

bin/client: obj/client.o
	$(CC) $(LDFLAGS) $^ -o $@

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f obj/* tmp/* output-folder/*
	rm -f bin/client bin/orchestrator