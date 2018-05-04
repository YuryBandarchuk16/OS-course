#include <dlfcn.h>
#include <cstdlib>
#include <cstdio>

#include "../static/print_sum.h"
#include "../static/add_numbers.h"

#include "../shared/print_sub.h"
#include "../shared/sub_numbers.h"


int main()
{
    print_sum(10, 20);
    print_sub(100, 200);

    void* extern_library = dlopen("./../libs/loadable_lib.so", RTLD_LAZY);

    if (extern_library == nullptr)
    {
        printf("%s\n", dlerror());
        return 0;
    }

	using func_t = void (*) (int, int);
	auto print_mul = reinterpret_cast<func_t>(dlsym(extern_library, "print_mul"));

    if (print_mul == nullptr)
    {
        printf("%s\n", dlerror());
        dlclose(extern_library);
		return 0;
    }

    print_mul(42, 17);
    dlclose(extern_library);
    return 0;
}
