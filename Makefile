
CC = g++
LIB = -pthread 

all: binder server 

server: libserver.cpp network/network.cpp message/message.cpp
	$(CC) $(LIB) -std=c++0x -o server network/network.cpp libserver.cpp message/message.cpp

binder: binder/binder.cpp
	$(CC) $(LIB) -o binder binder/binder.cpp
