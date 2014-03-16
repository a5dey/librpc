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

static int bindSockfd;

int sendExecuteToServer(locSucMsg *serverLoc, message msg)
{
    int servSockfd;
    void *rcvdMsg;
    struct addrinfo *serverInfo;
    char *IP = serverLoc->IP;
    char *port = (char*)malloc(INT_SIZE);
    convToByte(&(serverLoc->port), port, INT_SIZE, INT_SIZE); 
    serverInfo = getAddrInfo(IP, port);
    servSockfd = getSocket();
    if(servSockfd > 0)
    {
        if(connectSocket(servSockfd, serverInfo) > 0)
        {
            rcvdMsg = sendRecvBinder(servSockfd, msg);
        }
        else
        {
            EXIT_FAILURE;
            return -1;
        }
        if(rcvdMsg == 0)
            printf("Location Request failed\n");
        else
        {
            switch(((termMsg*)rcvdMsg)->type)
            {
                case EXECUTE_SUCCESS:
                    printf("EXECUTE REQUEST SUCCESS\n");
                    free(rcvdMsg);
                    return 0;
                case EXECUTE_FAILURE:
                    free(rcvdMsg);
                    return ((sucFailMsg*)rcvdMsg)->reason;
            }
        }
    }
    return 1;
}

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
    message exec_msg;
    openConnBinder();
    message msg;
    msg = createLocReqMsg(LOC_REQUEST, name, argTypes);
    assert( msg != NULL);
    void *rcvdMsg = sendRecvBinder(bindSockfd, msg);//sendRecvBinder needs to call createbndrMsg in message.cpp for recvFromEntity
    assert( rcvdMsg != NULL);
    if(rcvdMsg == 0)
        printf("Location Request failed\n");
    else
    {
        switch(((termMsg*)rcvdMsg)->type)
        {
            case LOC_SUCCESS:
                printf("LOCATION REQUEST SUCCESS\n");
                exec_msg = createExeSucMsg(EXECUTE, name, argTypes, args);
                return sendExecuteToServer((locSucMsg*)rcvdMsg, exec_msg);
            case LOC_FAILURE:
                return -1;
        }
    }
    return 1;
}
