#include "Executor.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>

class echo_serer_client {
public:
    static size_t const MAX_BUFF_SIZE = 1024;

    echo_serer_client(const char* ip_address, int port, const char* project_root_path):
            ip_address(ip_address), port(port), project_root_path(project_root_path) {
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

        w = new worker();
    }

    char* read_file(const char* path) {
        FILE* file = fopen(path, "rb");

        fseek(file, 0, SEEK_END);
        size_t size = static_cast<size_t>(ftell(file));

        char* content = new char[size];

        rewind(file);
        fread(content, sizeof(char), size, file);
        fclose(file);

        return content;
    }

    void run() {
        char buf[MAX_BUFF_SIZE];
        char response[MAX_BUFF_SIZE];

        while (true) {
            bzero(buf, MAX_BUFF_SIZE);
            bzero(response, MAX_BUFF_SIZE);

            std::vector<std::string> diff = w->get_diff();
            printf("Diff that will be uploaded:\n");
            for (auto file_path : diff) {
                printf("%s\n", file_path.c_str());
            }
            printf("---------------------------\n");

            for (auto file_path : diff) {
                const char* path = file_path.c_str();
                char* content = read_file(path);

                const size_t full_path_len = strlen(path) + strlen(project_root_path) + 10;
                char* full_path = new char[full_path_len];
                bzero(full_path, full_path_len);
                strcpy(full_path, project_root_path);
                strcat(full_path, path);

                char* bytes_cnt = new char[11];
                strcpy(bytes_cnt, std::to_string(strlen(content)).c_str());
                char* path_size = new char[11];
                strcpy(path_size, std::to_string(strlen(full_path)).c_str());

                printf("Sending %s...\n", path);
                write(socket_fd, bytes_cnt, 11);
                write(socket_fd, path_size, 11);
                write(socket_fd, full_path, strlen(full_path));
                write(socket_fd, content, strlen(content));

                read(socket_fd, response, MAX_BUFF_SIZE);
                printf("Server responded: %s\n", response);

                delete[] content;
                delete[] full_path;
                delete[] bytes_cnt;
                delete[] path_size;
            }

            printf("All the diff has been transferred\n");
            printf("Sleep for 2 seconds...\n");
            printf("---------------------------\n");
            printf("\n\n\n\n\n");
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    ~echo_serer_client() {
        delete w;
    }
private:
    const char* ip_address;
    const char* project_root_path;

    int port;

    int socket_fd;

    struct sockaddr_in client_address;

    const worker* w;

};

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage:\n");
        printf("      <ip_address> <port> <remote_project_root_path>\n");
        exit(0);
    }

    const char* ip_address = argv[1];
    int port = atoi(argv[2]);
    const char* project_root_path = argv[3];

    echo_serer_client client(ip_address, port, project_root_path);
    client.run();
    return 0;
}
