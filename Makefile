CC = g++
LIB = -pthread 

all: server client binder
binder: binder/binder.cpp network/network.cpp message/message.cpp
	$(CC) $(LIB) -std=c++0x -pthread -g -o bind binder/binder.cpp network/network.cpp message/message.cpp

server: libserver.cpp network/network.cpp message/message.cpp server.cpp
	$(CC) $(LIB) -std=c++0x -pthread -g -o server server.cpp network/network.cpp libserver.cpp message/message.cpp server_functions.cpp server_function_skels.cpp

client: libclient.cpp network/network.cpp message/message.cpp client.cpp
	$(CC) $(LIB) -std=c++0x -pthread -g -o client client.cpp network/network.cpp libclient.cpp message/message.cpp


