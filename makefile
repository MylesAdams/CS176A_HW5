CC = gcc
ARGS = -Wall

all: hangmen_server hangman_client

hangmen_server: hangman_server.c
	$(CC) $(ARGS) -o hangmen_server hangman_server.c

hangman_client: hangman_client.c
	$(CC) $(ARGS) -o hangman_client hangman_client.c

clean:
	rm -f *.o hangman_server *~
	rm -f *.o hangman_client *~
