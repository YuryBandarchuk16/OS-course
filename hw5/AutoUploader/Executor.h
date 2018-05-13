#include <iostream>
#include <cstring>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

#ifndef AUTOUPLOADER_EXECUTOR_H
#define AUTOUPLOADER_EXECUTOR_H

struct worker {
    void do_work(char* path, char* command_name, char* arg1, char* arg2, std::vector<std::string>& result) const {

        pid_t pid;
        int status;

        int link[2];
        char output[BUFFER_SIZE];

        if (pipe(link) == -1) {
            perror("pipe error");
        }

        pid = fork();
        if (pid == 0) {
            if (dup2(link[1], STDOUT_FILENO) == -1) {
                perror("dup2 error");
            };
            close(link[0]); close(link[1]);
            int ret_code;
            if ((ret_code = execl(path, command_name, arg1, arg2, (char*)0)) < 0) {
                if (ret_code < 0) {
                    perror("execl error");
                    exit(ret_code);
                } else {
                    exit(0);
                }
            }
            exit(0);
        } else if (pid > 0) {
            close(link[1]);
            ssize_t bytes_read = read(link[0], output, sizeof(output));

            std::string current = "";
            for (ssize_t i = 0; i < bytes_read; ++i) {
                bool skip = false;
                if (output[i] == '\r' && i + 1 < bytes_read && output[i + 1] == '\n') {
                    ++i;
                    skip = true;
                } else if (output[i] == '\n') {
                    skip = true;
                } else if (i + 1 >= bytes_read) {
                    skip = true;
                }
                if (skip) {
                    result.emplace_back(current);
                    current = "";
                } else {
                    current += output[i];
                }
            }
            wait(NULL);
        }
    }

    std::vector<std::string> get_diff() const {
        std::vector<std::string> result;
        do_work((char *) "/usr/local/bin/git", (char *) "git", (char *) "diff", (char *) "--name-only", result);
        return result;
    }

private:
    static int const BUFFER_SIZE = 4096;
};

#endif //AUTOUPLOADER_EXECUTOR_H
