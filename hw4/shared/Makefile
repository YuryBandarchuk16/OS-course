.PHONY: all clean

all: libshared.so

%.o: %.cpp
	g++ -std=c++11 -fPIC -c -o $@ $^

libshared.so: print_sub.o sub_numbers.o
	g++ -std=c++11 -shared -o ./../libs/libshared.so $^

clean:
	rm -rf ./../libs/shared_lib.so *.o


