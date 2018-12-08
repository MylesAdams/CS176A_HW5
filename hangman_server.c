#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#define BUFFER_SIZE 129
#define MAX_CLIENTS 3
#define WORD_LIST "./hangman_words.txt"
#define LOSE_STRING "You Lose.\n"
#define WIN_STRING "You Win!\n"
#define GAME_OVER_STRING "Game Over!"
#define OVERLOADED_STRING "server-overloaded"
#define WORD_WAS_STRING "The word was "

char LOSE_STRING_SIZE = 10;
char WIN_STRING_SIZE = 9;
char GAME_OVER_STRING_SIZE = 10;
char OVERLOADED_STRING_SIZE = 17;
char WORD_WAS_STRING_SIZE = 13;
char ZERO = 0;

int replace_chars(char* cur_str, char* word, char c);

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    perror("Invalid number of arguments");
    exit(EXIT_FAILURE);
  }

  srand(time(0));

  FILE* file;
  char* line = NULL;
  size_t len = 0;

  char* words[15];
  int num_words = 0;
  int client_words[3];
  char* client_strings[3];
  char* client_wrong_guesses[3];


  for (int i = 0; i < 15; ++i)
  {
    words[i] = malloc(9);
  }

  for (int i = 0; i < 3; ++i)
  {
    client_strings[i] = malloc(9);

    client_wrong_guesses[i] = malloc(7);
  }


  file = fopen(WORD_LIST, "r");

  if (file == NULL)
  {
    perror("Error reading file");
    exit(EXIT_FAILURE);
  }

  while (getline(&line, &len, file) != -1)
  {
    line[strcspn(line, "\n")] = 0;

    strncpy(words[num_words++], line, strlen(line) + 1);
  }

  fclose(file);

  if (num_words == 0)
  {
    printf("No words in hangman_words.txt\n");
    return 1;
  }

  int master_sockfd, client_sockfds[3] = {0}, cli_sockfd, client_games_started[3];
  struct sockaddr_in address;
  int port = strtol(argv[1], (char **)NULL, 10);

  fd_set read_fds;
  int max_sd;
  char msg_size;

  int num_clients = 0, sock_activity, msg_len, found;

  char buffer[BUFFER_SIZE];

  memset(buffer, '\0', 129);
  //memset(client_sockfds, 0, 3);

  if ((master_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("Failed to create socket");
    exit(EXIT_FAILURE);
  }

  memset(&address, 0, sizeof(address));

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(port);

  if (setsockopt(
          master_sockfd,
          SOL_SOCKET,
          SO_REUSEADDR,
          &(int){1}, sizeof(int)) < 0)
  {
    perror("Failed to set socket as reusable");
    exit(EXIT_FAILURE);
  }

  if (bind(
          master_sockfd,
          (const struct sockaddr *)& address,
          sizeof(address)) < 0)
  {
    perror("Failed to bind");
    exit(EXIT_FAILURE);
  }

  if (listen(master_sockfd, 3) < 0)
  {
    perror("Listening error");
    exit(EXIT_FAILURE);
  }

  int addr_len = sizeof(address);

  while(1)
  {
    FD_ZERO(&read_fds);

    FD_SET(master_sockfd, &read_fds);

    max_sd = master_sockfd;

    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
      if (client_sockfds[i] > 0)
      {
        FD_SET(client_sockfds[i], &read_fds);
      }

      if (client_sockfds[i] > max_sd)
      {
        max_sd = client_sockfds[i];
      }
    }

    sock_activity = select(max_sd + 1, &read_fds, NULL, NULL, NULL);

    if ((sock_activity < 0) && (errno != EINTR))
    {
      printf("Error on select\n");
    }

    if (FD_ISSET(master_sockfd, &read_fds))
    {
      if ((cli_sockfd = accept(
              master_sockfd,
              (struct sockaddr *)&address,
              (socklen_t*)&addr_len)) < 0)
      {
        perror("Failed to accept");
        exit(EXIT_FAILURE);
      }

      if (num_clients < MAX_CLIENTS)
      {
        for (int i = 0; i < MAX_CLIENTS; ++i)
        {
          if (client_sockfds[i] == 0)
          {
            client_sockfds[i] = cli_sockfd;

            int rand_val = rand() % num_words;

            int word_len = strlen(words[rand_val]);

            client_words[i] = rand_val;

            memset(client_wrong_guesses[i], '\0', sizeof(char) * 7);

            memset(client_strings[i], '_', sizeof(char) * word_len);

            memset(
                client_strings[i] + word_len,
                '\0',
                sizeof(char) * (9 - word_len));

            num_clients++;

            send(client_sockfds[i], &ZERO, 1, 0);

            send(client_sockfds[i], &word_len, 1, 0);

            send(client_sockfds[i], &ZERO, 1, 0);

            send(client_sockfds[i], client_strings[i], word_len, 0);

            break;
          }
        }
      }
      else
      {
        send(cli_sockfd, &OVERLOADED_STRING_SIZE, 1, 0);

        send(cli_sockfd, OVERLOADED_STRING, OVERLOADED_STRING_SIZE, 0);
      }
    }

    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
      if (FD_ISSET(client_sockfds[i], &read_fds))
      {
        if (recv(client_sockfds[i], buffer, 1, 0) == 0)
        {
          close(client_sockfds[i]);

          client_sockfds[i] = 0;

          client_games_started[i] = 0;

          num_clients--;
        }
        else
        {
          msg_len = buffer[0];

          if (msg_len != 0)
          {
            recv(client_sockfds[i], buffer, msg_len, 0);

            buffer[msg_len] = '\0';

            found = 0;

            if (strchr(client_strings[i], buffer[0]) == NULL)
            {
              found = replace_chars(client_strings[i], words[client_words[i]], buffer[0]);
            }

            if (found == 0)
            {
              for (int idx = 0; idx < 7; ++idx)
              {
                if (client_wrong_guesses[i][idx] == '\0')
                {
                  client_wrong_guesses[i][idx] = buffer[0];

                  break;
                }
              }
            }
          }

          if (strlen(client_wrong_guesses[i]) > 5)
          {
            msg_size = LOSE_STRING_SIZE + GAME_OVER_STRING_SIZE;

            send(client_sockfds[i], &msg_size, 1, 0);

            send(client_sockfds[i], LOSE_STRING, LOSE_STRING_SIZE, 0);

            send(client_sockfds[i], GAME_OVER_STRING, GAME_OVER_STRING_SIZE, 0);

            close(client_sockfds[i]);

            client_sockfds[i] = 0;

            client_games_started[i] = 0;

            num_clients--;
          }
          else if (strcmp(words[client_words[i]], client_strings[i]) == 0)
          {
            int msg_size =
                WORD_WAS_STRING_SIZE +
                strlen(client_strings[i]) +
                1 +
                WIN_STRING_SIZE +
                GAME_OVER_STRING_SIZE;

            send(client_sockfds[i], &msg_size, 1, 0);

            send(client_sockfds[i], WORD_WAS_STRING, WORD_WAS_STRING_SIZE, 0);

            send(
                client_sockfds[i],
                client_strings[i],
                strlen(client_strings[i]),
                0);

            send(client_sockfds[i], "\n", 1, 0);

            send(client_sockfds[i], WIN_STRING, WIN_STRING_SIZE, 0);

            send(client_sockfds[i], GAME_OVER_STRING, GAME_OVER_STRING_SIZE, 0);

            close(client_sockfds[i]);

            client_sockfds[i] = 0;

            client_games_started[i] = 0;

            num_clients--;
          }
          else
          {
            if (client_games_started[i] == 1)
            {
              int word_len = strlen(client_strings[i]);
              int num_wrong_guesses = strlen(client_wrong_guesses[i]);

              send(client_sockfds[i], &ZERO, 1, 0);

              send(client_sockfds[i], &word_len, 1, 0);

              send(client_sockfds[i], &num_wrong_guesses, 1, 0);

              send(client_sockfds[i], client_strings[i], word_len, 0);

              send(
                  client_sockfds[i],
                  client_wrong_guesses[i],
                  num_wrong_guesses,
                  0);
            }
            else
            {
              client_games_started[i] = 1;
            }
          }

        }
      }
    }
  }
}

int replace_chars(char* cur_str, char* word, char c)
{
  int found = 0;

  for (int i = 0; i < strlen(word); ++i)
  {
    if (word[i] == c)
    {
      cur_str[i] = c;

      found = 1;
    }
  }

  return found;
}

