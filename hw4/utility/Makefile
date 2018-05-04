.PHONY: all clean

all: utility

utility: main.o ./../static/static_lib.a ./../libs/libshared.so
	g++ -std=c++11 -L./../libs -lshared -Wl,-rpath,./../libs -o $@ $^

%.o: %.cpp
	g++ -std=c++11 -c -o $@ ./../static/static_lib.a $^

../static/static_lib.a:
	./build_static.sh

../shared/shared_lib.so:
	./build_shared.sh

clean:
	rm -rf utility *.o


