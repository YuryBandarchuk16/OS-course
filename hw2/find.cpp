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

using namespace std;

#define sz_filter_eq 1
#define sz_filter_less 2
#define sz_filter_grt 3
#define sz_filter_null 4
#define sz_filter_t char

struct filterable {
	virtual bool is_valid(struct stat*) = 0;
	virtual ~filterable() {};
};

struct inode_filter: public filterable {
	inode_filter(ino_t num): num(num) {};

	bool is_valid(struct stat* info) {
		return (info != NULL && info->st_ino == num);
	}

private:
	ino_t num;
};

struct size_filter: public filterable {
	size_filter(off_t size, sz_filter_t mode): size(size), mode(mode) {};

	bool is_valid(struct stat* info) {
		if (info == NULL) {
			return false;
		}

		switch (mode) {
			case sz_filter_eq:
				return (info->st_size == size);
			case sz_filter_less:
				return (info->st_size < size);
			case sz_filter_grt:
				return (info->st_size > size);
			default:
				return false;
		}
	}

private:
	off_t size;
	sz_filter_t mode;
};

struct nlinks_filter: public filterable {
	nlinks_filter(nlink_t num): num(num) {};

	bool is_valid(struct stat* info) {
		return (info != NULL && info->st_nlink == num);
	}

private:
	nlink_t num;
};

struct name_filter {
	name_filter(char* name): name(name) {};

	bool is_valid(char* file_path) {
		return (file_path != NULL && name != NULL && strcmp(basename(file_path), name) == 0);
	}

private:
	char* name;
};

struct worker {
	worker(char* path): path(path) {};
	
	void do_work(char* arg) {
		pid_t pid;
        int status;

        char* argv[] = {path, arg, 0};
        char* envp[] = {0};

        pid = fork();
        if (pid == 0) {
            int ret_code;
            if ((ret_code = execve(path, argv, envp)) < 0) {
                if (ret_code < 0) {
                    perror("execve error");
                    exit(ret_code);
                } else {
                    exit(0);
                }
            }
            exit(0);
        } else if (pid > 0) {
            pid_t w_res;
            while (1) {
                w_res = wait(&status);
                if (w_res == pid || w_res == -1) {
                    break;
                }
            }

            if (w_res == -1) {
                perror("wait error");
            }
		}
	}

private:
	char* path;
};

vector<char*> files;
vector<filterable*> filters;
name_filter* final_name_check = NULL;
worker* main_worker = NULL;

void clear_everything() {
	for (size_t i = 0; i < files.size(); ++i) {
		delete files[i];
	}

	for (size_t i = 0; i < filters.size(); ++i) {
		delete filters[i];
	}

	if (final_name_check != NULL) {
		delete final_name_check;
	}

	if (main_worker != NULL) {
		delete main_worker;
	}
}

void parse_arguments(char* option, char* value) {
	if (strcmp(option, "-inum") == 0) {
		filters.push_back(new inode_filter((ino_t)atoi(value)));
	} else if (strcmp(option, "-name") == 0) {
		if (final_name_check == NULL) {
			final_name_check = new name_filter(value);
		}
	} else if (strcmp(option, "-nlinks") == 0) {
		filters.push_back(new nlinks_filter((nlink_t)atoi(value)));
	} else if (strcmp(option, "-exec") == 0) {
		if (main_worker == NULL) {
			main_worker = new worker(value);
		}
	} else if (strcmp(option + 1, "size") == 0) {
		sz_filter_t sz_filter = sz_filter_null;
		if (option[0] == '-') {
			sz_filter = sz_filter_less;
		} else if (option[0] == '+') {
			sz_filter = sz_filter_grt;
		} else if (option[0] == '=') {
			sz_filter = sz_filter_eq;
		}
		if (sz_filter != sz_filter_null) {
			filters.push_back(new size_filter((off_t)atoi(value), sz_filter));
		}
	}
}

const char* separator = "/";

void dfs(char* file_path) {
	struct stat info;
	char buf[PATH_MAX + 1];
	if (stat(file_path, &info) == 0) {
		if (info.st_mode & S_IFDIR) {
  			struct dirent *ep;
  			DIR *dp = opendir(file_path);
  			if (dp != NULL) {
  				while ((ep = readdir(dp)) != NULL) {
  					if (strcmp(ep->d_name, "..") == 0 || strcmp(ep->d_name, ".") == 0) {
  						continue;
  					}
  					for (size_t i = 0; i <= PATH_MAX; i++) {
  						buf[i] = 0;
  					}
  					strcat(buf, file_path);
  					strcat(buf, separator);
  					strcat(buf, ep->d_name);
  					dfs(buf);
  				}
  			}
  			closedir(dp);
		} else {
			char* file_path_copy = new char[(int)strlen(file_path) + 2];
			strcpy(file_path_copy, file_path);
			files.push_back(file_path_copy);
		}
	}
}

bool match_all(struct stat* info) {
	for (size_t index = 0; index < filters.size(); ++index) {
		if (!filters[index]->is_valid(info)) {
			return false;
		}
	}
	return true;
}

int main(int argc, char** argv) {
	dfs(argv[1]);

	for (int index = 2; index < argc; index += 2) {
		parse_arguments(argv[index], argv[index + 1]);
	}

	vector<char*> validated_files;
	for (size_t index = 0; index < files.size(); ++index) {
		struct stat info;
		if (stat(files[index], &info) == 0) {
			if (match_all(&info)) {
				if (final_name_check != NULL && !final_name_check->is_valid(files[index])) {
					continue;
				} else {
					validated_files.push_back(files[index]);
				}
			}
		}
	}

	puts("Files matched:");
	for (size_t index = 0; index < validated_files.size(); ++index) {
		puts(validated_files[index]);
		if (main_worker != NULL) {
			puts("Sending to worker...");
			main_worker->do_work(validated_files[index]);
		}
	}

	clear_everything();
	return 0;
}
