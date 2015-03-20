CC = g++
CFLAG = -pthread -g -w -std=c++0x -c
FLAG = -pthread -g -w -std=c++0x

AR = ar rc
LIBRPC = librpc.a

SRCCLIENT = libclient.cpp
SRCSERVER = libserver.cpp


NET_DIR = network
MSG_DIR = message

NET_SRC = network.cpp
MSG_SRC = message.cpp

LIB_NET = network.o
LIB_MSG = message.o
LIB_CLI = libclient.o
LIB_SER = libserver.o
LIB_BIN = binder.o

all:
	$(CC) $(CFLAG) $(NET_DIR)/$(NET_SRC)
	$(CC) $(CFLAG) $(MSG_DIR)/$(MSG_SRC)
	$(CC) $(CFLAG) $(SRCCLIENT) 
	$(CC) $(CFLAG) $(SRCSERVER) 
	$(AR) $(LIBRPC) $(LIB_NET) $(LIB_MSG) $(LIB_CLI) $(LIB_SER)

	$(CC) $(CFLAG) binder/binder.cpp
	$(CC) $(FLAG) $(LIB_NET) $(LIB_MSG) $(LIB_BIN) -o bind

clean: rm *.o

