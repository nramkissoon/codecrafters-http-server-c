#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
typedef uint64_t u64;
typedef int64_t i64;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint8_t u8;
typedef int8_t i8;
typedef float f32;
typedef double f64;

enum http_request {
  HTTP_REQUEST_GET,
  HTTP_REQUEST_POST,
  HTTP_REQUEST_PUT,
  HTTP_REQUEST_PATCH,
  HTTP_REQUEST_DELETE,
  HTTP_REQUEST__COUNT,
};

struct http_header {
	enum http_request method;
	u8 *path;
	u8 user_agent[1024];
	u8 content_type[1024];
};

struct method_string_pair {
	enum http_request method;
	u8 *str;
} known_methods[] = {
	{HTTP_REQUEST_GET, "GET"},       {HTTP_REQUEST_POST, "POST"},
    {HTTP_REQUEST_PUT, "PUT"},       {HTTP_REQUEST_PATCH, "PATCH"},
    {HTTP_REQUEST_DELETE, "DELETE"},
};

enum http_request method_from_str(u8 *str) {
	for (u32 i = 0; i < sizeof(known_methods); i++) {
		if (strcmp(known_methods[i].str, str)) {
			return known_methods[i].method;
		}
	}
	return -1;
}

bool startsWith(const char* pre, const char* str) {
	size_t lenpre = strlen(pre), lenstr = strlen(str);
	return lenstr < lenpre ? false : memcmp(pre, str, lenpre) == 0;
}

int main() {
	// Disable output buffering
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);

	i32 server_fd, client_addr_len;
	struct sockaddr_in client_addr;
	
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}
	
	// Since the tester restarts your program quite often, setting SO_REUSEADDR
	// ensures that we don't run into 'Address already in use' errors
	i32 reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEADDR failed: %s \n", strerror(errno));
		return 1;
	}
	
	struct sockaddr_in serv_addr = { .sin_family = AF_INET ,
									 .sin_port = htons(4221),
									 .sin_addr = { htonl(INADDR_ANY) },
									};
	
	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}
	
	i32 connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0) {
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}
	
	printf("Waiting for a client to connect...\n");
	client_addr_len = sizeof(client_addr);
	
	u32 connection = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
	printf("Client connected\n");

	u8 buffer[1024];
  	read(connection, buffer, sizeof(buffer));

	struct http_header request = {0};
	request.method = method_from_str(strtok(buffer, " "));
	request.path = strtok(0, " ");
	printf("path: %s\n", request.path);

	if (strcmp(request.path, "/") == 0) {
		char ok_200[] = "HTTP/1.1 200 OK\r\n\r\n";
		send(connection, ok_200, sizeof(ok_200), 0);
	} else if (startsWith("/echo/", request.path)) {
		u8 *echo = request.path + 6; // point to the char after the prefix route /echo/
		printf("echo: %s\n", echo);
		u32 len = strlen(echo);
		printf("echo len: %d\n", len);
		char response[1024];
		sprintf(response,
            "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: "
            "%ld\r\n\r\n%s",
            strlen(echo), echo);
    	send(connection, response, strlen(response), 0);
	} else if (strncmp(request.path, "/user-agent", 11) == 0) {
		strtok(0, "\r\n");
		strtok(0, "\r\n");
		char *userAgent = strtok(0, "\r\n") + 12;
		const char *format = "HTTP/1.1 200 OK\r\nContent-Type: "
							"text/plain\r\nContent-Length: %zu\r\n\r\n%s";
		char response[1024];
		sprintf(response, format, strlen(userAgent), userAgent);
		send(connection, response, sizeof(response), 0);
	} else {
		char not_found_404[] = "HTTP/1.1 404 Not Found\r\n\r\n";
    	send(connection, not_found_404, sizeof(not_found_404), 0);
	}
	
	close(connection);
	close(server_fd);

	return 0;
}
