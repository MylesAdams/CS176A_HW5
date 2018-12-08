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
#include <ctype.h>

#define BUFFER_SIZE 129

char ZERO = 0;

void space_format(char* buffer);

int main(int argc, char** argv)
{
  if (argc < 3)
  {
    perror("Invalid number of arguments");
    exit(EXIT_FAILURE);
  }

  int sockfd;
  struct sockaddr_in serv_addr;
  int port = strtol(argv[2], (char **)NULL, 10);

  char buffer[BUFFER_SIZE] = {0};
  char input_buffer[BUFFER_SIZE] = {0};

  int recv_val, word_len, num_incorrect, game_started = 0;

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
  }

  memset(&serv_addr, 0, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  serv_addr.sin_addr.s_addr = inet_addr(argv[1]);

  if (connect(sockfd, (struct sockaddr *)& serv_addr, sizeof(serv_addr)) != 0)
  {
    perror("Failed to connect to server");
    exit(EXIT_FAILURE);
  }

  while(1)
  {

    recv(sockfd, buffer, 1, 0);

    recv_val = buffer[0];

    if (recv_val == 0)
    {
      // Word Length
      recv(sockfd, buffer, 1, 0);

      word_len = buffer[0];

      // Num Incorrect
      recv(sockfd, buffer, 1, 0);

      num_incorrect = buffer[0];

      // Word
      recv(sockfd, buffer, word_len, 0);

      buffer[word_len] = '\0';

      space_format(buffer);

      if (game_started == 1)
      {
        printf("%s\n", buffer);

        // Incorrect Guesses
        printf("Incorrect Guesses: ");

        if (num_incorrect > 0)
        {
          recv(sockfd, buffer, num_incorrect, 0);

          buffer[num_incorrect] = '\0';

          printf("%s", buffer);
        }

        printf("\n\n");
      }


    }
    else
    {
      recv(sockfd, buffer, recv_val, 0);

      buffer[recv_val] = '\0';

      printf("%s\n", buffer);

      return 0;
    }

    if (game_started == 0)
    {
      while(1)
      {
        printf("Ready to start game? (y/n): ");

        fgets(input_buffer, BUFFER_SIZE, stdin);

        if (input_buffer[0] == 'y')
        {
          send(sockfd, &ZERO, 1, 0);

          game_started = 1;

          printf("%s\n", buffer);

          printf("Incorrect Guesses: \n\n");

          break;
        }
        else if (input_buffer[0] == 'n')
        {
          close(sockfd);

          return 1;
        }
        else
        {
          printf("Invalid input\n");
        }
      }
    }

    while(1)
    {
      printf("Letter to guess: ");

      fgets(input_buffer, BUFFER_SIZE, stdin);

      input_buffer[strcspn(input_buffer, "\n")] = 0;

      if (strlen(input_buffer) == 1 && isalpha(input_buffer[0]))
      {
        input_buffer[1] = tolower(input_buffer[0]);

        input_buffer[0] = 1;

        send(sockfd, input_buffer, 2, 0);

        break;
      }
      else
      {
        printf("Error! Please guess one letter.\n");
      }
    }
  }



}

void space_format(char* buffer)
{
  char formatted_buffer[30];
  int i;

  for (i = 0; i < strlen(buffer); ++i)
  {
    formatted_buffer[2 * i] = buffer[i];
    formatted_buffer[2 * i + 1] = ' ';
  }

  formatted_buffer[2 * (i - 1) + 1] = '\0';

  strncpy(buffer, formatted_buffer, strlen(formatted_buffer) + 1);
}
