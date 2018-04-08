#include <stdio.h>
#include <sys/stat.h>

using namespace std;

#define sz_filter_eq 1
#define sz_filter_less 2
#define sz_filter_grt 3
#define sz_filter_t char

struct filterable {
	virtual bool is_valid(struct stat* info) = 0;
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

int main(int argc, char** argv) {
	char* abs_path = argv[1];

	return 0;
}
