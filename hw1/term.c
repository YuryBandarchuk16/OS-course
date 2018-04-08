#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"

#define MAX_ARG_LENGTH 100
#define MAX_TERM_STMT_LENGTH 1000

void init() {
	printf("itmo_term>");
}

void set_default_color() {
	printf("%s", KNRM);
}

size_t get_argc(char * s, size_t len) {
	size_t ret = 0;

	size_t i;
	for (i = 0; i < len; ++i) {
		if (s[i] == '\t') {
			s[i] = ' ';
		}
	}

	char skip_spaces = 0;

	for (i = 0; i < len; ++i) {
		if (s[i] == '\"') {
			skip_spaces ^= 1;
		}
		if (skip_spaces == 0 && s[i] != ' ' && (i + 1 == len || s[i + 1] == ' ')) {
			++ret;
		}
	}

	return ret;
}


char** get_argv(const char * s, size_t len, size_t argc) {
	char** ret = malloc(sizeof(char*) * (argc + 1));

	size_t i;
	for (i = 0; i < argc; ++i) {
		ret[i] = malloc(sizeof(char) * (MAX_ARG_LENGTH + 1));
	}

	size_t s_i = 0;
	for (i = 0; i < argc; ++i) {
		size_t a_i = 0;
		while (s_i < len && s[s_i] == ' ') { 
			++s_i; 
		}

		char term_char = ' ';
		if (s[s_i] == '\"') {
			++s_i;
			term_char = '\"';
		}

		while (s_i < len && s[s_i] != term_char) {
			ret[i][a_i++] = s[s_i++];
		}
		if (term_char == '\"') {
			++s_i;
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
				buf[buf_sz] = '\0';
				break;
			} else {
				buf[buf_sz++] = nxt_c;
			}
		}
		if (rd < 0) {
			if (rd == EOF) {
				puts("");
				break;
			}
			perror("scanf error");
		}
		
		if (buf_sz == 0) {
			continue;
		}

		size_t argc = get_argc(buf, buf_sz);
		char **argv = get_argv(buf, buf_sz, argc);
		
		if (argc == 1) {
			if (strcmp(argv[0], "itmo") == 0) {	
				printf("ITMO!!!!!\n");
				continue;
			}
			if (strcmp(argv[0], "exit") == 0) {
				printf("%sOK, Bye...\n%s", KBLU, KNRM);
				exit(0);
			}
		}

		pid_t pid;
		int status;

		pid = fork();
		if (pid == 0) {
			int ret_code;
			puts(argv[0]);
			if ((ret_code = execve(argv[0], argv, envp)) < 0) {
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
			
			printf("[%s%s%s] %sfinished with status:%s %s%d\n%s", KMAG, buf, KNRM, KBLU, KNRM, status == 0 ? KGRN : KRED, status, KNRM);

			size_t i;
			for (i = 0; i < argc; ++i) {
				free(argv[i]);
			}
			free(argv);
		} else {
			perror("fork error");
			exit(1);
		}
	}

	return 0;
}
