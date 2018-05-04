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

class echo_serer_client {
public:
    static size_t const MAX_BUFF_SIZE = 1024;

    echo_serer_client(const char* ip_address, int port):
            ip_address(ip_address), port(port) {
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd < 0) {
            perror("socket error:");
        }

        if (port < 0 || port >= 65536) {
            printf("invalid port");
            exit(0);
        }

        bzero(&client_address, sizeof(client_address));

        client_address.sin_family = AF_INET;
        client_address.sin_port = htons(port);
        inet_pton(AF_INET, ip_address, &client_address.sin_addr);

        if (connect(socket_fd, reinterpret_cast<struct sockaddr*>(&client_address), sizeof(client_address)) < 0) {
            perror("connect error:");
        }
    }

    void run() {
        char buf[MAX_BUFF_SIZE];
        char response[MAX_BUFF_SIZE];

        while (true) {
            bzero(buf, MAX_BUFF_SIZE);
            bzero(response, MAX_BUFF_SIZE);

            printf("Enter a message:\n");
            fgets(buf, MAX_BUFF_SIZE, stdin);

            write(socket_fd, buf, strlen(buf));
            printf("Sent: %s", buf);

            read(socket_fd, response, MAX_BUFF_SIZE);
            printf("Received: %s", response);
        }
    }
private:
    const char* ip_address;

    int port;

    int socket_fd;

    struct sockaddr_in client_address;

};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage:\n");
        printf("      <ip_address> <port>\n");
        exit(0);
    }

    char const* ip_address = argv[1];
    int port = atoi(argv[2]);

    echo_serer_client client(ip_address, port);
    client.run();
    return 0;
}