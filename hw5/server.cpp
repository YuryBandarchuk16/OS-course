#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

class echo_server {
public:
    static size_t const MAX_BUFF_SIZE = 1024;

    echo_server (const char* ip_address, int port, int max_connections_num):
            ip_address(ip_address), port(port), max_conn(max_connections_num), _started(false) {
        listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd < 0) {
            perror("socket error");
            exit(0);
        }

        if (port < 0 || port >= 65536) {
            printf("invalid port");
            exit(0);
        }

        bzero(&server_address, sizeof(server_address));
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(port);
        inet_aton(ip_address, &server_address.sin_addr);

        if (bind(listen_fd, reinterpret_cast<struct sockaddr*>(&server_address), sizeof(server_address)) < 0) {
            perror("bind error");
            exit(0);
        }

        if (listen(listen_fd, max_conn) < 0) {
            perror("listen error");
            exit(0);
        }

        _started = true;
    }

    void run() {
        if (!_started) {
            printf("can not run the server as it was not configured");
            exit(0);
        }

        char message[MAX_BUFF_SIZE];
        bzero(message, MAX_BUFF_SIZE);

        printf("Server is running...\n");

        while (true) {
            comm_fd = accept(listen_fd, reinterpret_cast<struct sockaddr*>(NULL), NULL);
            if (comm_fd < 0) {
                perror("accept error");
                continue;
            }

            while (read(comm_fd, message, MAX_BUFF_SIZE) > 0) {
                write(comm_fd, message, strlen(message));
                bzero(message, MAX_BUFF_SIZE);
            }

            close(comm_fd);
        }
    }

private:
    const char* ip_address;

    int port;
    int max_conn;

    int comm_fd;
    int listen_fd;

    struct sockaddr_in server_address;

    bool _started;
};

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage:\n");
        printf("      <ip_address> <port> <max_connections_num>\n");
        exit(0);
    }

    char const* ip_address = argv[1];
    int port = atoi(argv[2]);
    int max_connections_num = atoi(argv[3]);

    echo_server server(ip_address, port, max_connections_num);
    server.run();
    return 0;
}
