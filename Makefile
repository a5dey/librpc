CC = g++
LIB = -pthread 

server: libserver.cpp network/network.cpp message/message.cpp
	$(CC) $(LIB) -std=c++0x -g -o server server.cpp network/network.cpp libserver.cpp message/message.cpp
binder: binder/binder.cpp network/network.cpp message/message.cpp
	$(CC) $(LIB) -g -o binder binder/binder.cpp network/network.cpp message/message.cpp

