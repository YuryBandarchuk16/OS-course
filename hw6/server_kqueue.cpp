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
#include <sys/event.h>
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

		int kqueue_fd = kqueue();
		if (kqueue_fd < 0) {
			perror("kqueue error");
			close(listen_fd);
			exit(0);
		}
		
		struct kevent have[max_conn + 2];   
        
		for (size_t i = 0; i <= max_conn; ++i) {
			bzero(&have[i], sizeof(struct kevent));
			EV_SET(&have[i], listen_fd, EVFILT_READ, EV_ADD, 0, 0, 0);
		}

		kevent(kqueue_fd, have, max_conn + 1, NULL, 0, NULL);

        while (true) {
			bzero(have, sizeof(have));
			int res = kevent(kqueue_fd, NULL, 0, have, max_conn + 1, NULL);
            if (res < 0) {
				perror("kevent error");
				continue;
			}

			for (size_t i = 0; i <= max_conn; ++i) {
				if (have[i].filter != EVFILT_READ) {
					continue;
				}

				if (have[i].ident == listen_fd) {
					int rfd = accept(listen_fd, 0, 0);
					if (rfd < 0) {
						perror("accept error");
						continue;
					} else {
						set_nonblocking(rfd);
						bzero(&have[i], sizeof(struct kevent));
						EV_SET(&have[i], rfd, EVFILT_READ, EV_ADD, 0, 0, 0);
						if (-1 == kevent(kqueue_fd, &have[i], 1, NULL, 0, NULL)) {
							perror("kevent error");
							exit(0);
						} else {
							r_fds.insert(rfd);
						}
					}
				} else {
                    static const int input_sz = 2048;
                    static char message[input_sz];
                    int received = recv(have[i].ident, message, input_sz, 0);
                    if (received == 0 && errno != EAGAIN) {
                        shutdown(have[i].ident, SHUT_RDWR);
                        close(have[i].ident);
                    } else if (received > 0) {
                        int cur_fd = have[i].ident;
                        for (set<int> :: iterator it = r_fds.begin(); it != r_fds.end(); ++it) {
                            if (*it != cur_fd) {
                                send(*it, message, received, 0);
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
