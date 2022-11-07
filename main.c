#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

const int PORT = 8080;
const int MAX_CONNS = 1;
const int PROCS = 1;

void now(char * time_s) {
	time_t now = time(NULL);
	struct tm * p = localtime(&now);
	strftime(time_s, 128, "%H:%M:%S %a %d %b %Z", p);
}

int main() {
	printf("Starting...\n");

	int sock = socket(AF_INET, SOCK_STREAM, 0);

	if (sock < 0) {
		printf("Failed creating socket...\n");
		return 0;
	}

	const int enable = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		printf("setsockopt(SO_REUSEADDR) failed");
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

	for (int p = 0; p < PROCS; p++) {
		int pid = fork();
		if (pid == 0) break;
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

		struct timeval request_start;
		gettimeofday(&request_start, NULL);

		char buffer[1024] = {0};

		int valread = read(connection, buffer, 1024);
		/* printf("%s\n", buffer); */

		char *method = strtok(buffer, " ");
		char *path = strtok(NULL, " ");

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

		if (strstr(path, ".js") != NULL) {
			content_type = "Content-Type: text/javascript\n";
		}

		if (strstr(path, ".png") != NULL) {
			content_type = "Content-Type: image/png\n";
		}

		int found = 0;
		long content_length = 0;
		char filepath [9 + strlen(path)];
		snprintf(filepath, 9 + strlen(path), "./public%s", path);
		FILE * file = fopen(filepath, "r");

		if (!file) {
			content = "file not found";
		} else {
			fseek(file, 0, SEEK_END);
			content_length = ftell(file);
			fseek(file, 0, SEEK_SET);
			content = malloc(content_length);
			if (content) {
				found = 1;
				int n = fread(content, sizeof(char), content_length, file);
				content[n] = '\0';
			} else {
				content = "file not found";
			}
		}

		printf("%s len:%lu", content, strlen(content)); // why is this wrong for png

		char * response_header = 0;
		if (found == 1) {
			response_header = "HTTP/1.1 200 OK\n";
		} else {
			response_header = "HTTP/1.1 404 Not Found\n";
		}
		char * go_away_google = "Permissions-Policy: interest-cohort=()\n";

		char response_content_length [128];
		snprintf(response_content_length, 128, "Content-Length: %ld\n", content_length);

		int buffer_size = 128 + strlen(response_header) + content_length;

		printf("buffer size: %i ", buffer_size);

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

		struct timeval request_end;
		gettimeofday(&request_end, NULL);

		char time[128];
		now(time);

		float response_time = (request_end.tv_sec - request_start.tv_sec) * 1000.0f + (request_end.tv_usec - request_start.tv_usec) / 1000.0f;

		printf("[ %s ] %s %s %ld bytes took %fms\n", time, method, path, content_length, response_time);

		close(connection);
	}

	return 0;
}
