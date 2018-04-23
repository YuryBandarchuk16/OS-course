#include <iostream>
#include <cstdio>
#include <sys/mman.h>
#include <unistd.h>

using namespace std;

const size_t code_size = 16;
unsigned char code[] = {
        0x55, 0x48, 0x89, 0xe5, 0xc7, 0x45, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x8b, 0x45, 0xfc, 0x5d, 0xc3
};

using ret_t = int(*)();

struct memory_helper {
  static size_t calculate_needed_size(size_t len) {
    size_t page_size = sysconf(_SC_PAGE_SIZE);
    size_t pages_cnt = 1;

    while (len > pages_cnt * page_size) {
      ++pages_cnt;
    }

    return page_size * pages_cnt;
  }

  static void unmap(void* ptr, size_t size) {
      int res = munmap(ptr, size);
      if (0 != res) {
          perror("munmap error");
          exit(0);
      }
  }

  static void* map(size_t len, int fd, size_t offset) {
    void* res = mmap(nullptr, len * sizeof(unsigned char),
                    PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS,
                    fd, offset);
    if (res == (void*) -1) {
      perror("mmap error");
      exit(0);
    }

    return res;
  }

  static void protect(void* ptr, size_t len, int prot) {
    int res = mprotect(ptr, len, prot);
    if (0 != res) {
      perror("protection settings error");
      memory_helper::unmap(ptr, len);
      exit(0);
    }
  }
};

void patch_code(int value) {
    code[7] = (value & 0xFF) >> 0;
    code[8] = (value & 0xFF00) >> 8;
    code[9] = (value & 0xFF0000) >> 16;
    code[10] = (value & 0xFF000000) >> 24;
}

void just_do() {
  size_t needed_code_size = memory_helper::calculate_needed_size(code_size);
  void* ptr = memory_helper::map(needed_code_size, -1, 0);

  memcpy(ptr, code, code_size);

  memory_helper::protect(ptr, needed_code_size, PROT_READ | PROT_EXEC);

  ret_t func = (ret_t)ptr;
  printf("%d\n", func());

  memory_helper::protect(ptr, needed_code_size, PROT_READ | PROT_WRITE);
  memory_helper::unmap(ptr, needed_code_size);
}

int main() {
    while (true) {
      int x;
      scanf("%d", &x);

      patch_code(x);

      just_do();
    }
    return 0;
}
