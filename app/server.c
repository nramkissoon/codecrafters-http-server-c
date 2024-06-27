#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define HTTP_PROTOCOL_11 "HTTP/1.1"
#define HEADER_SEP ": "
#define CRLF "\r\n"
static char *directory_name;
typedef struct request_data request_data;
typedef struct headers_data headers_data;
typedef struct header header;
struct header {
  char *name;
  char *value;
};
struct headers_data {
  int length;
  header **headers;
};
struct request_data {
  char *method, *path, *body;
  headers_data *headers;
};
char **get_str_tokens(char *str, char *delim);
int str_starts_with(const char *str, char *searchStr);
char *build_header(const char *name, const char *val);
char *build_start_line(int status, const char *status_text);
void parse_request_buffer(char *buff, request_data *result);
void *http_handler(void *args) {
  char *res;
  char buff[255];
  int client = (int)args;
  read(client, buff, sizeof(buff));
  printf("%s", buff);
  request_data *request = malloc(sizeof(struct request_data));
  parse_request_buffer(buff, request);
  if (strcmp(request->method, "POST") == 0 &&
      str_starts_with(request->path, "/files")) {
    char *file_name = get_str_tokens(request->path, "/")[1];
    size_t file_path_size = strlen(directory_name) + 1 + strlen(file_name) + 1;
    char *file_path = calloc(sizeof(char), file_path_size);
    strcat(file_path, directory_name);
    strcat(file_path, "/");
    strcat(file_path, file_name);
    char *file_contents = strdup(request->body);
    FILE *file = fopen(file_path, "w");
    fputs(file_contents, file);
    fclose(file);
    char *res_start = build_start_line(201, "Created");
    res = calloc(sizeof(char), strlen(res_start) + strlen(CRLF) + 1);
    strcat(res, res_start);
    strcat(res, CRLF);
  } else if (strcmp(request->method, "GET") == 0 &&
             str_starts_with(request->path, "/files")) {
    char *file_name = get_str_tokens(request->path, "/")[1];
    size_t file_path_size = strlen(directory_name) + 1 + strlen(file_name) + 1;
    char *file_path = calloc(sizeof(char), file_path_size);
    strcat(file_path, directory_name);
    strcat(file_path, "/");
    strcat(file_path, file_name);
    char *file_contents = NULL;
    if (access(file_path, 0) == 0) {
      FILE *file = fopen(file_path, "rb");
      fseek(file, 0, SEEK_END);
      long file_length = ftell(file);
      file_contents = malloc(sizeof(char) * file_length + 1);
      fseek(file, 0, SEEK_SET);
      fread(file_contents, file_length, 1, file);
      fclose(file);
      file_contents[file_length] = '\0';
      char *res_start = build_start_line(200, "OK");
      char *content_type_header =
          build_header("Content-Type", "application/octet-stream");
      char *content_length_header_value = malloc(32 * sizeof(char));
      snprintf(content_length_header_value, 32, "%lu", file_length);
      char *content_length_header =
          build_header("Content-Length", content_length_header_value);
      size_t res_size = strlen(res_start);
      res_size += strlen(content_type_header);
      res_size += strlen(content_length_header);
      res_size += strlen(CRLF);
      res_size += strlen(file_contents);
      res_size += 1;
      char *res_local = calloc(sizeof(char), res_size);
      strcat(res_local, res_start);
      strcat(res_local, content_type_header);
      strcat(res_local, content_length_header);
      strcat(res_local, CRLF);
      strcat(res_local, file_contents);
      res = res_local;
      printf("xd %s", res);
    } else {
      res = "HTTP/1.1 404 Not Found\r\n\r\n";
    }
  } else if (str_starts_with(request->path, "/user-agent")) {
    char *user_agent_header_value = NULL;
    for (int i = 0; i < request->headers->length; ++i) {
      if (strcmp(request->headers->headers[i]->name, "User-Agent") == 0) {
        user_agent_header_value = request->headers->headers[i]->value;
      }
    }
    char *content_type_header = build_header("Content-Type", "text/plain");
    char *content_length_header;
    if (user_agent_header_value != NULL) {
      char *content_length_header_value =
          malloc(strlen(user_agent_header_value) * sizeof(char));
      snprintf(content_length_header_value, strlen(user_agent_header_value),
               "%lu", strlen(user_agent_header_value));
      content_length_header =
          build_header("Content-Length", content_length_header_value);
    } else {
      content_length_header = build_header("Content-Length", "0");
    }
    const char *resStart = build_start_line(200, "OK");
    size_t res_size = strlen(resStart);
    res_size += strlen(content_type_header);
    res_size += strlen(content_length_header);
    res_size += strlen(CRLF);
    if (user_agent_header_value != NULL) {
      res_size += strlen(user_agent_header_value);
    }
    res_size += 1;
    char *res_local = calloc(sizeof(char), res_size);
    strcat(res_local, resStart);
    strcat(res_local, content_type_header);
    strcat(res_local, content_length_header);
    strcat(res_local, CRLF);
    if (user_agent_header_value != NULL) {
      strcat(res_local, user_agent_header_value);
    }
    res = res_local;
    printf("xd %s", res);
  } else if (str_starts_with(request->path, "/echo")) {
    char *echoStr = get_str_tokens(request->path, "/")[1];
    char *contentTypeHeader = build_header("Content-Type", "text/plain");
    char *contentLengthValueStr = malloc(32 * sizeof(char));
    snprintf(contentLengthValueStr, 32, "%lu", strlen(echoStr));
    char *contentLengthHeader =
        build_header("Content-Length", contentLengthValueStr);
    const char *resStart = build_start_line(200, "OK");
    size_t res_size = strlen(resStart);
    res_size += strlen(contentTypeHeader);
    res_size += strlen(contentLengthHeader);
    res_size += strlen(CRLF);
    res_size += strlen(echoStr);
    res_size += 1;
    char *res_local = calloc(sizeof(char), res_size);
    strcat(res_local, resStart);
    strcat(res_local, contentTypeHeader);
    strcat(res_local, contentLengthHeader);
    strcat(res_local, CRLF);
    strcat(res_local, echoStr);
    res = res_local;
    printf("xd %s", res);
  } else if (strcmp("/", request->path) != 0) {
    res = "HTTP/1.1 404 Not Found\r\n\r\n";
  } else {
    res = "HTTP/1.1 200 OK\r\n\r\n";
  }
  write(client, res, strlen(res));
  free(request);
}
int main(int arg_ct, char **args) {
  for (int i = 1; i < arg_ct; ++i) {
    if (strcmp("--directory", args[i]) == 0 && arg_ct > i + 1) {
      directory_name = args[i + 1];
    }
  }
  // Disable output buffering
  setbuf(stdout, NULL);
  // You can use print statements as follows for debugging, they'll be visible
  // when running tests.
  printf("Logs from your program will appear here!\n");
  int server_fd, client_addr_len;
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
  struct sockaddr_in client_addr;
  pthread_t tid;
  while (1) {
    client_addr_len = sizeof(client_addr);
    int client =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client < 0) {
      break;
    }
    printf("Client connected\n");
    pthread_create(&tid, NULL, http_handler, (void *)client);
  }
  close(server_fd);
  return 0;
}
void parse_request_buffer(char *buff, request_data *result) {
  char *rest = strdup(buff);
  memset(result, 0, sizeof(struct request_data));
  size_t method_len = strcspn(rest, " ");
  result->method = calloc(sizeof(char), method_len + 1);
  memcpy(result->method, rest, &rest[method_len] - rest);
  rest += method_len + 1;
  size_t path_len = strcspn(rest, " ");
  result->path = calloc(sizeof(char), path_len + 1);
  memcpy(result->path, rest, &rest[path_len] - rest);
  rest += path_len + 1;
  // skip version and CRLF
  size_t version_len = strcspn(rest, CRLF);
  rest += version_len + 2;
  int headers_len = 0;
  struct headers_data *headers_data = malloc(sizeof(struct headers_data));
  struct header **headers = malloc(0);
  while (rest[0] != '\r' || rest[1] != '\n') {
    headers_len++;
    void *new_headers = realloc(headers, sizeof(struct header *) * headers_len);
    if (new_headers == NULL) {
      perror("");
    } else {
      headers = (header **)new_headers;
    }
    size_t name_len = strcspn(rest, ":");
    char *name = calloc(sizeof(char), name_len + 1);
    memcpy(name, rest, &rest[name_len] - rest);
    name[name_len] = '\0';
    rest += name_len + 1;
    while (isspace(*rest))
      rest++;
    size_t value_len = strcspn(rest, CRLF);
    char *value = calloc(sizeof(char), value_len + 1);
    memcpy(value, rest, &rest[value_len] - rest);
    value[value_len] = '\0';
    rest += value_len + 2;
    headers[headers_len - 1] = malloc(sizeof(header *));
    headers[headers_len - 1]->name = name;
    headers[headers_len - 1]->value = value;
  }
  headers_data->headers = headers;
  headers_data->length = headers_len;
  result->headers = headers_data;
  result->body = strdup(rest) + 2;
}
char **get_str_tokens(char *str, char *delim) {
  char **parts = malloc(5 * sizeof(*parts));
  char *token;
  char *rest = strdup(str);
  for (int i = 0; (token = strtok_r(rest, delim, &rest)); i++) {
    parts[i] = calloc(sizeof(char), strlen(token));
    strcpy(parts[i], token);
  }
  return parts;
}
int str_starts_with(const char *str, char *searchStr) {
  if (strncmp(str, searchStr, strlen(searchStr)) == 0)
    return 1;
  else
    return 0;
}
char *build_header(const char *name, const char *val) {
  size_t header_size =
      (strlen(name) + strlen(HEADER_SEP) + strlen(val) + strlen(CRLF) + 1);
  char *header = calloc(sizeof(char), header_size);
  strcat(header, name);
  strcat(header, HEADER_SEP);
  strcat(header, val);
  strcat(header, CRLF);
  return header;
}
char *build_start_line(int status, const char *status_text) {
  char *status_num_str = malloc(6 * sizeof(char));
  snprintf(status_num_str, 6, "%d", status);
  size_t start_line_size =
      (strlen(HTTP_PROTOCOL_11) + 1 + strlen(status_num_str) + 1 +
       strlen(status_text) + strlen(CRLF) + 1);
  char *start_line = calloc(sizeof(char), start_line_size);
  strcat(start_line, HTTP_PROTOCOL_11);
  strcat(start_line, " ");
  strcat(start_line, status_num_str);
  strcat(start_line, " ");
  strcat(start_line, status_text);
  strcat(start_line, CRLF);
  return start_line;
}