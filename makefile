CC = gcc
ARGS = -Wall
CCFLAGS = -g3 -gdwarf-2

all: hangman_server hangman_client

debug: CCFLAGS += -DDEBUG -g
debug: hangman_server hangman_client

hangman_server: hangman_server.c
	$(CC) $(CCFLAGS) $(ARGS) -o hangman_server hangman_server.c

hangman_client: hangman_client.c
	$(CC) $(CCFLAGS) $(ARGS) -o hangman_client hangman_client.c

clean:
	rm -f *.o hangman_server *~
	rm -f *.o hangman_client *~
