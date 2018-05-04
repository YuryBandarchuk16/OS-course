.PHONY: all clean

all: loadable_lib.so

%.o: %.cpp
	g++ -std=c++11 -fPIC -c -o $@ $^

loadable_lib.so: print_mul.o mul_numbers.o
	g++ -std=c++11 -shared -o ./../libs/loadable_lib.so $^

clean:
	rm -rf ./../libs/loadable_lib.so *.o


