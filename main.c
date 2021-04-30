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

    char * content = 0;
    char * content_type = "Content-Type: text/text\n";

    if (strcmp(path, "/") == 0) {
      path = "/index.html";
    }

    if (strstr(path, ".html") != NULL) {
      content_type = "Content-Type: text/html\n";
    }

    if (strstr(path, ".css") != NULL) {
      content_type = "Content-Type: text/css\n";
    }

    int found = 0;
    char filepath [9 + strlen(path)];
    snprintf(filepath, 9 + strlen(path), "./public%s", path);
    FILE * file = fopen(filepath, "r");
    if (!file) {
      content = "file not found";
    } else {
      fseek(file, 0, SEEK_END);
      long length = ftell(file);
      fseek(file, 0, SEEK_SET);
      content = malloc(length);
      if (content) {
        found = 1;
        fread(content, 1, length, file);
      } else {
        content = "file not found";
      }
    }

    int content_length = strlen(content);

    char * response_header = 0;
    if (found == 1) {
      response_header = "HTTP/1.1 200 OK\n";
    } else {
      response_header = "HTTP/1.1 404 Not Found\n";
    }
    char * go_away_google = "Permissions-Policy: interest-cohort=()\n";

    char response_content_length [128];
    snprintf(response_content_length, 128, "Content-Length: %d", content_length);

    int buffer_size = 128 + strlen(response_header) + content_length;
    char response [buffer_size];

    snprintf(
        response,
        buffer_size,
        "%s%s%s%s\n\n%s",
        response_header,
        content_type,
        response_content_length,
        go_away_google,
        content
        );

    write(connection, response, strlen(response));

    close(connection);
  }

  return 0;
}
