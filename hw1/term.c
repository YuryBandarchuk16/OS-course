#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_ARG_LENGTH 100
#define MAX_TERM_STMT_LENGTH 1000

void init() {
	printf("itmo_term>");
}

size_t get_argc(char * s, size_t len) {
	size_t ret = 0;

	size_t i;
	for (i = 0; i < len; ++i) {
		if (s[i] == '\t') {
			s[i] = ' ';
		}
	}
	for (i = 0; i < len; ++i) {
		if (s[i] != ' ' && (i + 1 == len || s[i + 1] == ' ')) {
			++ret;
		}
	}

	if (len >= 2 && s[len - 1] == '&' && s[len - 2] == ' ') {
		--ret;
	}

	return ret;
}

char lst_exec = 0;

char** get_argv(const char * s, size_t len, size_t argc) {
	char** ret = malloc(sizeof(char*) * (argc + 1));

	size_t i;
	for (i = 0; i < argc; ++i) {
		ret[i] = malloc(sizeof(char) * (MAX_ARG_LENGTH + 1));
	}

	lst_exec = 0;

	size_t s_i = 0;
	for (i = 0; i < argc; ++i) {
		size_t a_i = 0;
		while (s_i < len && s[s_i] == ' ') { 
			++s_i; 
		}
		if (i == 0 && s_i + 1 < len && s[s_i] == '.' && s[s_i + 1] == '/') {
			s_i += 2;
			lst_exec = 1;
		}
		while (s_i < len && s[s_i] != ' ') {
			ret[i][a_i++] = s[s_i++];
		}
		ret[i][a_i] = '\0';
	}
	ret[argc] = NULL;

	return ret;
}

int main(int argc, char** argv, char** envp) {
    char buf[MAX_TERM_STMT_LENGTH];

	// main loop
	while (1) {
		init();
		
		int rd;
		char nxt_c;
		size_t buf_sz = 0;
		
		while ((rd = scanf("%c", &nxt_c)) > 0) {
			if (nxt_c == '\n') {
				break;
			} else {
				buf[buf_sz++] = nxt_c;
			}
		}
		if (rd < 0) {
			perror("scanf error");
		}
		
		size_t argc = get_argc(buf, buf_sz);
		char **argv = get_argv(buf, buf_sz, argc);
		
		if (argc == 1) {
			if (strcmp(argv[0], "itmo") == 0) {	
				printf("ITMO Ebet!\n");
				continue;
			}
			if (strcmp(argv[0], "exit") == 0) {
				printf("OK, Bye...\n");
				exit(0);
			}
		}

		pid_t pid;
		int status;
		
		pid = fork();
		if (pid == 0) {
			int ret_code;
			if ((ret_code = execvp(argv[0], argv)) < 0) {
				if (lst_exec == 1) {
					ret_code = execve(argv[0], argv + 1, envp);
				}
				if (ret_code < 0) {
					perror("error");
				}
			}
		} else if (pid > 0) {
			while (wait(&status) != pid);
		} else {
			perror("fork error");
		}
	}

	return 0;
}
