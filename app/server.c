#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define BUFFER_SIZE 4098
char *get_path(char *request);
char *get_user_agent(char *request);

int main(int argc, char **argv) {
  char *directory = NULL;
  if (argc >= 2 && (strncmp(argv[1], "--directory", 11) == 0)) {
    directory = argv[2];
  }
  // Disable output buffering
  setbuf(stdout, NULL);
  // You can use print statements as follows for debugging, they'll be visible
  // when running tests.
  printf("Logs from your program will appear here!\n");
  int server_fd, client_addr_len;
  struct sockaddr_in client_addr;
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    printf("Socket creation failed: %s...\n", strerror(errno));
    return 1;
  }
  // Since the tester restarts your program quite often, setting REUSE_PORT
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) <
      0) {
    printf("SO_REUSEPORT failed: %s \n", strerror(errno));
    return 1;
  }
  struct sockaddr_in serv_addr = {
      .sin_family = AF_INET,
      .sin_port = htons(4221),
      .sin_addr = {htonl(INADDR_ANY)},
  };
  if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
    printf("Bind failed: %s \n", strerror(errno));
    return 1;
  }
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    printf("Listen failed: %s \n", strerror(errno));
    return 1;
  }
  printf("Waiting for a client to connect...\n");
  client_addr_len = sizeof(client_addr);
  int client_id;
  while (1) {
    client_id =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    printf("Client connected\n");
    int pid = fork();
    if (pid == 0) {
      break;
    }
  }
  char request[1024] = {};
  ssize_t size = recv(client_id, request, 1024, 0);
  if (size <= 0) {
    printf("Invalid request\n");
    return 0;
  }
  char *path = get_path(request);
  char *response = NULL;
  if (strcmp(path, "/") == 0) {
    response = "HTTP/1.1 200 OK\r\n\r\n";
  } else if (strncmp(path, "/echo/", 6) == 0) {
    char *body = path + 6;
    size_t contentLength = strlen(path) - 6;
    char *format = "HTTP/1.1 200 OK\r\n"
                   "Content-Type: text/plain\r\n"
                   "Content-Length: %zu\r\n\r\n%s";
    response = malloc(512);
    sprintf(response, format, contentLength, body);
  } else if (strncmp(path, "/user-agent", 11) == 0) {
    char *user_agent = get_user_agent(request);
    size_t contentLength = strlen(user_agent);
    char *format = "HTTP/1.1 200 OK\r\n"
                   "Content-Type: text/plain\r\n"
                   "Content-Length: %zu\r\n\r\n%s";
    response = malloc(512);
    sprintf(response, format, contentLength, user_agent);
  } else if (strncmp(path, "/files/", 7) == 0) {
    char *file = strchr(path + 1, '/');
    if (file != NULL) {
      char *filepath = strcat(directory, file);
      FILE *fd = fopen(filepath, "r");
      if (fd != NULL) {
        char *buffer[BUFFER_SIZE] = {0};
        int bytes_read = fread(buffer, 1, BUFFER_SIZE, fd);
        char *format = "HTTP/1.1 200 OK\r\n"
                       "Content-Type: application/octet-stream\r\n"
                       "Content-Length: %zu\r\n\r\n%s";
        if (bytes_read > 0) {
          response = malloc(BUFFER_SIZE);
          sprintf(response, format, bytes_read, buffer);
        }
      } else {
        response = "HTTP/1.1 404 Not Found\r\n\r\n";
      }
    }
  } else {
    response = "HTTP/1.1 404 Not Found\r\n\r\n";
  }
  int responseSize = strlen(response);
  if (write(client_id, response, responseSize) != responseSize) {
    printf("Unable to write to the socket");
  }
  close(client_id);
  close(server_fd);
  return 0;
}
char *get_path(char *request) {
  char *start = strchr(request, ' ');
  char *end = strchr(start + 1, ' ');
  int startPos = start - request;
  int endPos = end - request;
  int length = endPos - startPos - 1;
  char *path = malloc(length);
  memcpy(path, start + 1, length);
  path[length] = '\0';
  return path;
}
char *get_user_agent(char *request) {
  char *start = request;
  char *currentRequest = NULL;
  for (int i = 0; i < 4; i++) {
    currentRequest = start;
    start = strchr(currentRequest, ' ') + 1;
  }
  char *end = strchr(start, '\r');
  int length = end - start;
  char *user_agent = malloc(length);
  memcpy(user_agent, start, length);
  user_agent[length] = '\0';
  return user_agent;
}