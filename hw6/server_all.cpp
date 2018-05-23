#if defined (__unix__)
	#define LINUX kek
#endif

#include <iostream>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <iterator>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <sys/types.h>
#ifdef LINUX
#include <sys/epoll.h>
#endif
#include <sys/socket.h>
#ifndef LINUX
#include <sys/event.h>
#endif
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <poll.h>
#include <set>

#define GLOBAL_MAX_CONN 128

using namespace std;

class echo_server {
public:
    static size_t const MAX_BUFF_SIZE = 1024;

    echo_server (int port, int max_connections_num):
            port(port), max_conn(min(max_connections_num, GLOBAL_MAX_CONN)), _started(false) {
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

				set_main_fd();

				init();

      while (true) {
				int events_num = get_events_num();
			for (int i = 0; i <= events_num; ++i) {
#ifndef LINUX
					if (have[i].filter != EVFILT_READ) {
						continue;
					}
				if (have[i].ident == listen_fd) {
					accept_new_connection(i);
					continue;
				}
#else
				if (have[i].data.fd == listen_fd) {
					accept_new_connection(i);
				}
#endif
				process(get_fd(i));
      }
		}
	}

		void process(int fd) {
			static const int input_sz = 2048;
			static char message[input_sz];
			int received = recv(fd, message, input_sz, 0);
			if (received == 0 && errno != EAGAIN) {
					shutdown(fd, SHUT_RDWR);
					close(fd);
			} else if (received > 0) {
					int cur_fd = fd;
					for (set<int> :: iterator it = r_fds.begin(); it != r_fds.end(); ++it) {
							if (*it != cur_fd) {
									send(*it, message, received, 0);
							}
					}
			}
		}

		int get_fd(int i) {
#ifndef LINUX
			return have[i].ident;
#else
			return have[i].data.fd;
#endif
		}

		int get_events_num() {
				int result = 0;
#ifndef LINUX
				bzero(have, sizeof(have));
				result = kevent(main_fd, NULL, 0, have, max_conn + 1, NULL);
				if (result < 0) {
					perror("kevent error");
				}
#else
				result = epoll_wait(main_fd, have, max_conn, -1);
				if (result < 0) {
						perror("epoll_wait error");
				}
#endif
				return result;
		}

		void init() {
#ifndef LINUX
			for (size_t i = 0; i <= max_conn; ++i) {
				bzero(&have[i], sizeof(struct kevent));
				EV_SET(&have[i], listen_fd, EVFILT_READ, EV_ADD, 0, 0, 0);
			}
			kevent(main_fd, have, max_conn + 1, NULL, 0, NULL);
#else
			struct epoll_event server_epoll;
			server_epoll.data.fd = listen_fd;
			server_epoll.events = EPOLLIN;
			epoll_ctl(main_fd, EPOLL_CTL_ADD, listen_fd, &server_epoll);
#endif
		}

		bool accept_new_connection(int i) {
			int rfd = accept(listen_fd, 0, 0);
			printf("accepted fd = %d\n", rfd);
			if (rfd < 0) {
				perror("accept error");
				return false;
			} else {
				set_nonblocking(rfd);
#ifndef LINUX
				bzero(&have[i], sizeof(struct kevent));
				EV_SET(&have[i], rfd, EVFILT_READ, EV_ADD, 0, 0, 0);
				if (-1 == kevent(main_fd, &have[i], 1, NULL, 0, NULL)) {
					perror("kevent error");
					return false;
				}
#else
				struct epoll_event connection_epoll;
				connection_epoll.data.fd = rfd;
				connection_epoll.events = EPOLLIN;
				if (epoll_ctl(main_fd, EPOLL_CTL_ADD, rfd, &connection_epoll) == -1) {
	 				perror("epoll_ctl error");
					return false;
				}
#endif
				r_fds.insert(rfd);
			}
			return true;
		}

		void set_main_fd() {
			#ifndef LINUX
			main_fd = kqueue();
			if (main_fd < 0) {
				perror("kqueue error");
			}
			#else
			main_fd = epoll_create(max_conn + 5);
			if (main_fd < 0) {
				perror("epoll create error");
			}
			#endif
			if (main_fd < 0) {
				close(listen_fd);
				exit(0);
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

		int main_fd;

#ifndef LINUX
		struct kevent have[GLOBAL_MAX_CONN];
#else
		struct epoll_event have[GLOBAL_MAX_CONN];
#endif

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
