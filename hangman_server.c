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
#define LOSE_STRING "You Lose."
#define WIN_STRING "You Win!"
#define GAME_OVER_STRING "Game Over!"
#define OVERLOADED_STRING "server-overloaded"
#define WORD_WAS_STRING "The word was "

char WIN_STRING_SIZE = 8;
char LOSE_STRING_SIZE = 9;
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
    words[i] = malloc(9 * sizeof(char));
  }

  for (int i = 0; i < 3; ++i)
  {
    client_strings[i] = malloc(9 * sizeof(char));
    client_wrong_guesses[i] = malloc(7 * sizeof(char));
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

  int master_sockfd, client_sockfds[3], cli_sockfd;
  struct sockaddr_in address;
  int port = strtol(argv[1], (char **)NULL, 10);

  fd_set read_fds;
  int max_sd;

  int num_clients = 0, sock_activity, recv_size, msg_len;

  char in_buffer[BUFFER_SIZE];
  char out_buffer[BUFFER_SIZE];

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
            printf("%s\n", client_strings[i]);
            memset(client_strings[i] + word_len, '\0', sizeof(char) * (9 - word_len));
            printf("%s\n", client_strings[i]);

            num_clients++;

            send(cli_sockfd, &ZERO, 1, 0);

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
        if ((recv_size = recv(client_sockfds[i], in_buffer, 1, 0)) == 0)
        {
          close(client_sockfds[i]);
          FD_CLR(client_sockfds[i], &read_fds);
          num_clients--;
        }
        else
        {
          in_buffer[1] = '\0';

          msg_len = in_buffer[0];

          recv(client_sockfds[i], in_buffer, msg_len, 0);
          in_buffer[msg_len] = '\0';

          printf("%s\n", client_strings[i]);

          printf("debug5\n");

          int found = replace_chars(client_strings[i], words[client_words[i]], in_buffer[0]);

          printf("debug6\n");

          printf("%s\n", client_strings[i]);

          if (found == 0)
          {
            for (int idx = 0; i < 7; ++i)
            {
              if (client_wrong_guesses[i][idx] == '\0')
              {
                client_wrong_guesses[i][idx] = in_buffer[0];
                break;
              }
            }
          }

          if (strlen(client_wrong_guesses[i]) > 5)
          {
            send(client_sockfds[i], &LOSE_STRING_SIZE, 1, 0);
            send(client_sockfds[i], LOSE_STRING, LOSE_STRING_SIZE, 0);

            send(client_sockfds[i], &GAME_OVER_STRING_SIZE, 1, 0);
            send(client_sockfds[i], GAME_OVER_STRING, GAME_OVER_STRING_SIZE, 0);
          }
          else if (strcmp(words[client_words[i]], client_strings[i]) == 0)
          {
            int word_msg_size = WORD_WAS_STRING_SIZE + strlen(client_strings[i]);

            send(client_sockfds[i], &word_msg_size, 1, 0);
            send(
                client_sockfds[i],
                *WORD_WAS_STRING + client_strings[i],
                word_msg_size, 0);

            send(client_sockfds[i], &WIN_STRING_SIZE, 1, 0);
            send(client_sockfds[i], WIN_STRING, WIN_STRING_SIZE, 0);

            send(client_sockfds[i], &GAME_OVER_STRING_SIZE, 1, 0);
            send(client_sockfds[i], GAME_OVER_STRING, GAME_OVER_STRING_SIZE, 0);
          }
          else
          {
            int word_len = strlen(client_strings[i]);
            int num_wrong_guesses = strlen(client_wrong_guesses[i]);

            send(client_sockfds[i], &ZERO, 1, 0);
            send(client_sockfds[i], &word_len, 1, 0);
            send(client_sockfds[i], &num_wrong_guesses, 1, 0);

            send(client_sockfds[i], client_strings[i], word_len, 0);
            send(client_sockfds[i], client_wrong_guesses[i], num_wrong_guesses, 0);
          }

        }
      }
    }
  }
}

int replace_chars(char* cur_str, char* word, char c)
{
  int found = 0;

  printf("funcdebug1\n");
  for (int i = 0; i < strlen(word); ++i)
  {
    if (word[i] == c)
    {
      printf("funcdebug2\n");
      cur_str[i] = c;
      found = 1;
      printf("funcdebug3\n");
    }
  }

  return found;
}
