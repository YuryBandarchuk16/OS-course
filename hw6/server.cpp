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

using namespace std;

class echo_server {
public:
    static size_t const MAX_BUFF_SIZE = 1024;

    echo_server (int port, int max_connections_num):
            port(port), max_conn(max_connections_num), _started(false) {
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
        //inet_aton(ip_address, &server_address.sin_addr);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(listen_fd, reinterpret_cast<struct sockaddr*>(&server_address), sizeof(server_address)) < 0) {
            perror("bind error");
            exit(0);
        }

        set_nonblocking(listen_fd);

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

        struct pollfd have[max_conn + 2];
        have[0].fd = listen_fd;
        have[0].events = POLLIN;

        while (true) {
            size_t size = 1;
            for (set<int> :: iterator it = r_fds.begin(); it != r_fds.end(); ++it) {
                have[size].fd = *it;
                have[size++].events = POLLIN;
            }
            poll(have, size, -1);
            for (size_t i = 0; i < size; ++i) {
                bool new_event = (have[i].revents & POLLIN) == POLLIN;
                if (!new_event) {
                    continue;
                }
                if (i == 0) {
                    int rfd = accept(listen_fd, 0, 0);
                    if (rfd < 0) {
                        perror("accept error");
                        continue;
                    } else {
                        set_nonblocking(rfd);
                        r_fds.insert(rfd);
                    }
                } else {
                    static const int input_sz = 2048;
                    static char message[input_sz];
                    int received = recv(have[i].fd, message, input_sz, MSG_NOSIGNAL);
                    if (received == 0 && errno != EAGAIN) {
                        shutdown(have[i].fd, SHUT_RDWR);
                        close(have[i].fd);
                        set<int> :: iterator it = r_fds.find(have[i].fd);
                        if (it != r_fds.end()) {
                            r_fds.erase(it);
                        }
                    } else if (received > 0) {
                        for (size_t j = 1; j < size; ++j) {
                            if (i != j) {
                                send(have[j].fd, message, received, MSG_NOSIGNAL);
                            }
                        }
                    }
                }
            }
        }
    }

private:
    const char* ip_address;

    int port;
    int max_conn;

    int listen_fd;

    struct sockaddr_in server_address;

    bool _started;

    set<int> r_fds;

    int set_nonblocking(int fd) {
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) {
            flags = 0;
        }
        return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage:\n");
        printf("      <port> <max_connections_num>\n");
        exit(0);
    }

    int port = atoi(argv[1]);
    int max_connections_num = atoi(argv[2]);

    echo_server server(port, max_connections_num);
    server.run();
    return 0;
}