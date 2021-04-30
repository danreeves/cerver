#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

const int PORT = 8080;
const int MAX_CONNS = 1024;

void now(char * time_s) {
    time_t now = time(NULL);
    struct tm * p = localtime(&now);
    strftime(time_s, 1000, "%H:%M:%S %a %d %b %Z", p);
}

int main() {
  printf("Starting...\n");

  int sock = socket(AF_INET, SOCK_STREAM, 0);

  if (sock < 0) {
    printf("Failed creating socket...\n");
    return 0;
  }

  struct sockaddr_in address;

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);
  int addrlen = sizeof(address);

  if (bind(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
    printf("Failed to bind socket...\n");
    return 0;
  }

  if (listen(sock, MAX_CONNS) < 0) {
    printf("Failed to listen to socket...\n");
    return 0;
  }

  while(1) {
    int connection = accept(
        sock,
        (struct sockaddr *)&address,
        (socklen_t*)&addrlen
    );

    if (connection < 0) {
      printf("Failed to accept connection...\n");
      close(connection);
      continue;
    }

    char buffer[1024] = {0};

    int valread = read(connection, buffer, 1024);
    /* printf("%s\n", buffer); */

    char *method = strtok(buffer, " ");
    char *path = strtok(NULL, " ");

    char time_s[100];
    now(time_s);

    printf("[ %s ] %s %s\n", time_s, method, path);

    if (valread < 0) {
      printf("No bytes to read in buffer\n");
    }

    char *response = "HTTP/1.1 200 OK\n"
                  "Permissions-Policy: interest-cohort=()"
                  "Content-Type: text/plain\n"
                  "Content-Length: 14\n"
                  "\n"
                  "Hello, planet!";

    write(connection, response, strlen(response));

    close(connection);
  }

  return 0;
}
