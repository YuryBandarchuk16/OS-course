#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <tuple>

struct copier {
	static void copy1(char* src, char* dst, int size) {
		if (size > 0) {
			asm volatile(
			"foreach_1:\n"
				"mov $0, %%rax\n"
				"mov (%0), %%al\n"
				"mov %%al, (%1)\n"
				"inc %0\n"
				"inc %1\n"
				"dec %2\n"
				"cmp $0, %2\n"
				"jne foreach_1\n"
					:"+r"(src), "+r"(dst), "+r"(size)
					:"r"(size)
					:"cc", "memory", "rax"
			);
		}
	}
	static void copy8(char* src, char* dst, int size) {
		if (size > 0) {
			int blocks_cnt = size / 8;
			int rem = size % 8;
			if (blocks_cnt > 0) {
				asm volatile(
				"foreach_8:\n"
					"mov $0, %%rax\n"
					"mov (%0), %%rax\n"
					"mov %%rax, (%1)\n"
					"add $8, %0\n"
					"add $8, %1\n"
					"dec %2\n"
					"cmp $0, %2\n"
					"jne foreach_8\n"
						:"+r"(src), "+r"(dst), "+r"(blocks_cnt)
						:"r"(size)
						:"cc", "memory", "rax"
				);
				src += (blocks_cnt * 8);
				dst += (blocks_cnt * 8);
			}
			if (rem > 0) {
				copier::copy1(src, dst, rem);
			}
		}
	}
	static void copy16(char* src, char*dst, int size) {
		if (size > 0) {
			int blocks_cnt = size / 16;
			int rem = size % 16;
			if (blocks_cnt > 0) {
				asm volatile(
				"foreach_16:\n"
					"xorps %%xmm0, %%xmm0\n"
					"movdqu (%0), %%xmm0\n"
					"movdqu %%xmm0, (%1)\n"
					"add $16, %0\n"
					"add $16, %1\n"
					"dec %2\n"
					"cmp $0, %2\n"
					"jne foreach_16\n"
						:"+r"(src), "+r"(dst), "+r"(blocks_cnt)
						:"r"(size)
						:"cc", "memory", "xmm0"
				);
				src += (blocks_cnt * 8);
				dst += (blocks_cnt * 8);
			}
			if (rem > 0) {
				copier::copy8(src, dst, rem);
			}
		}
	}
	static std::tuple<char*, char*, int> align_and_copy(char* src, char*dst, int size) {
		int addr = (int&)(src);
		int off = addr % 16;
		copier::copy8(src, dst, off);
		return std::make_tuple(src + off, dst + off, size - off);
	}
	static void copy16_fast(char* src, char*dst, int size) {
		if (size > 0) {
			std::tuple<char*, char*, int> t = copier::align_and_copy(src, dst, size);
			copier::copy16(std::get<0>(t), std::get<1>(t), std::get<2>(t));
		}
	}
};

void test_copy1() {
	char s1[6] = {'1', 'a', '3', 'e', 'b', '\0'};
	char s2[6] = {'x', 'y', 'z', 'w', 'e', '\0'};
	copier::copy1(s1, s2, 6);
	for (int i = 0; i < 6; ++i) {
		if (s1[i] != s2[i]) {
			puts("test_copy1: FAIL");
			return;
		}
	}
	puts("test_copy1: OK");
}

void test_copy8() {
	char s1[11] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '\0'};
	char s2[11] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', '\0'};
	copier::copy8(s2, s1, 10);
	for (int i = 0; i < 10; ++i) {
		if (s2[i] != s1[i]) {
			puts("test_copy8: FAIL");
			return;
		}
	}
	puts("test_copy8: OK");
}

void test_copy16() {
	srand(94718203);
	char *s1 = new char[12345678];
	char *s2 = new char[12345678];
	for (int i = 0; i < 12345678; ++i) {
		s1[i] = (char)(rand() % 10 + 48);
		s2[i] = (char)(rand() % 10 + 48);
	}
	double start_time = clock();
	copier::copy16(s1, s2, 12345678);
	double end_time = clock();
	double total_time = (end_time - start_time) / CLOCKS_PER_SEC;
	for (int i = 0; i < 12345678; ++i) {
		if (s1[i] != s2[i]) {
			puts("test_copy16: FAIL");
			delete[] s1;
			delete[] s2;
			return;
		}
	}
	puts("test_copy16: OK");
	printf("Total time: %.15f\n", total_time);
	delete[] s1;
	delete[] s2;
}

void test_copy16_fast() {
	srand(382009357);
	char *s1 = new char[12345678];
	char *s2 = new char[12345678];
	for (int i = 0; i < 12345678; ++i) {
		s1[i] = (char)(rand() % 10 + 48);
		s2[i] = (char)(rand() % 10 + 48);
	}
	double start_time = clock();
	copier::copy16_fast(s1, s2, 12345678);
	double end_time = clock();
	double total_time = (end_time - start_time) / CLOCKS_PER_SEC;
	for (int i = 0; i < 12345678; ++i) {
		if (s1[i] != s2[i]) {
			puts("test_copy16_fast: FAIL");
			delete[] s1;
			delete[] s2;
			return;
		}
	}
	puts("test_copy16_fast: OK");
	printf("Total time: %.15f\n", total_time);
	delete[] s1;
	delete[] s2;
}

void run_tests() {
	test_copy1();
	test_copy8();
	test_copy16();
	test_copy16_fast();
	test_copy16();
}
 
int main() {
	run_tests();
    return 0;
}
