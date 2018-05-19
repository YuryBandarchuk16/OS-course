#include <iostream>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <iterator>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <poll.h>
#include <set>

class echo_serer_client {
public:
    static size_t const MAX_BUFF_SIZE = 1024;

    echo_serer_client(const char* ip_address, int port):
            ip_address(ip_address), port(port) {
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd < 0) {
            perror("socket error");
            exit(0);
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
            perror("connect error");
            exit(0);
        }
    }

    void run() {
        char buf[MAX_BUFF_SIZE];
        char response[MAX_BUFF_SIZE];

        struct pollfd have[2];
        have[0].fd = socket_fd;
        have[0].events = POLLIN;

        bool last_is_read = false;
        while (true) {
            printf("Would you like to Read or Write to the char? (R/W):");
            char opt;
            if (last_is_read == true) {
                scanf("\n");
            }
            last_is_read = false;
            scanf("%c", &opt);

            bzero(buf, MAX_BUFF_SIZE);
            bzero(response, MAX_BUFF_SIZE);

            if (opt == 'W') {
                printf("Enter a message:\n");
                scanf("\n");
                fgets(buf, MAX_BUFF_SIZE, stdin);
                write(socket_fd, buf, strlen(buf));
                printf("Sent: %s", buf);
            } else if (opt == 'R') {
                last_is_read = true;
                poll(have, 1, -1);
                if ((have[0].revents & POLLIN) == POLLIN) {
                    read(socket_fd, response, MAX_BUFF_SIZE);
                    printf("Received: %s", response);
                }
            } else {
                printf("Invalid option, the chat AI does not undestand what do you want!\n");
             }
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
