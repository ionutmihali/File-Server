.PHONY: clean

all: serv cli

serv: server.c
	gcc server.c -o server -lpthread

cli: client.c
	gcc client.c -o client

clean: rm server client