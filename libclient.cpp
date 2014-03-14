#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <netinet/in.h>
#include <pthread.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include "network/network.h"
#include "librpc.h"
#include "message/message.cpp"

static int sockfd;
static int bindSockfd;

int openConnBinder()
{
    struct addrinfo *binderInfo;
    char *binderIP = getenv("BINDER_ADDRESS");
    char *binderPort = getenv("BINDER_PORT");
    binderInfo = getAddrInfo(binderIP, binderPort);
    bindSockfd = getSocket();
    if(bindSockfd > 0)
    {
        if(connectSocket(bindSockfd, binderInfo) > 0)
        {
            return 1;
        }
    }
    
    return 0;
}

int rpcCall(char *name, int *argTypes, void **args)
{
    char *IP;
    int port;
    message exec_msg;
    struct addrinfo *serverInfo;
    openConnBinder();
    message msg;
    msg = createLocReqMsg(LOC_REQUEST, name, argTypes);
    void *clientMsg = sendRecvBinder(bindSockfd, msg);//sendRecvBinder needs to call createbndrMsg in message.cpp for recvFromEntity
    void *prsdMsg = parseMsg((message)clientMsg, getLengthOfMsg((message)clientMsg));//binder will set the type to LOC_SUCCESS or LOC_FAILURE
    
    locSucMsg *m = (locSucMsg *)prsdMsg;
    if (m->type == LOC_SUCCESS)
    {
        IP = m->IP;
        port = m->port;
        message msg_exec = createExeSucMsg(EXECUTE, name, argTypes, args);
        serverInfo = getAddrInfo(IP, (char *)port);
        sockfd = getSocket();
        if(sockfd > 0)
        {
            if(connectSocket(sockfd, serverInfo) > 0)
            {
                sendToEntity(sockfd, msg_exec);
                exec_msg = (message)recvFromEntity(sockfd);
                void *parsed_execute = parseMsg(exec_msg, getLengthOfMsg(exec_msg));
            }
            else
            {
                EXIT_FAILURE;
                return -1;
            }
            return 0;
        }
    }
    return 1;
}