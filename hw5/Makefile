all: server client

server: server.cpp
    g++ -o out/server server.cpp -std=c++11 -Wall -Wextra -fsanitize=undefined -ggdb

client: client.cpp
    g++ -o out/client client.cpp -std=c++11 -Wall -Wextra -fsanitize=undefined -ggdb