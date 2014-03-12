/*
 * Shreya Agrawal
 */
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
#include <sys/types.h>
#include <sys/time.h>
#include <map>

struct cmp_skeleArgs{
    bool operator()(skeleArgs a, skeleArgs b)
    {
        return strcmp(a.name, b.name) < 0;
    }
};

static int sockfd;
static int bindSockfd;

static addrInfo myName;
static addrinfo *myInfo;
static addrinfo *binderInfo;
static std::map<skeleArgs, skeleton, cmp_skeleArgs> serverStore;

static bool terminate;

int handleExecute(exeMsg *msg, int _sockfd)
{
    skeleArgs *key;
    message byteMsgSent;
    key = createFuncArgs(msg->name, msg->argTypes);
    std::map<skeleArgs, skeleton, cmp_skeleArgs>::iterator it;
    it = serverStore.find(*key);
    if(it == serverStore.end())
        return -1;

    int (*func)(int *argTypes, void **args) = NULL;
    func = it->second;
    int reason = func(msg->argTypes, msg->args);
    if (reason < 0)
    {
        //sucFailMsg *sentMsg = new sucFailMsg;
        //sentMsg->type = EXECUTE_FAILURE;
        //sentMsg->reason = reason;
        byteMsgSent = createSucFailMsg(EXECUTE_FAILURE, reason);
    }
    else
    {
        //exeMsg *sentMsg = new exeMsg;
        //sentMsg = msg;
        //sentMsg->type = EXECUTE_SUCCESS;
        byteMsgSent = createExeSucMsg(EXECUTE_SUCCESS, msg->name, msg->argTypes, msg->args);
        
    }
    if(sendToEntity(_sockfd, byteMsgSent) < 0)
    {
        perror("Reply to EXECUTE failed");
        return -1;
    }
    return 1;
}

int handleTerminate(int _sockfd)
{
    close(sockfd);
    close(bindSockfd);
    exit(0);
}

int handleIncomingConn(int _sockfd)
{
    void* rcvdMsg = recvFromEntity(sockfd);
    message retMsg;
    switch(((termMsg*)rcvdMsg)->type)
    {
        case EXECUTE:
            return handleExecute((exeMsg*)rcvdMsg, _sockfd);
            break;
        case TERMINATE:
            return handleTerminate(_sockfd);
            break;
        default:
            retMsg = createTermMsg(MESSAGE_INVALID);
    }
    if(sendToEntity(_sockfd, retMsg) < 0)
    {
        perror("Reply to EXECUTE failed");
        return -1;
    }
    return 1;
}

int listen()
{
    myInfo = getAddrInfo(NULL, PORT);
    sockfd = getSocket();
    if(sockfd > 0)
    {
        if(bindSocket(sockfd, myInfo))
        {
            if(listenSocket(sockfd))
            {
                strcpy(myName.IP, getMyIP());
                printf("SERVER_ADDRESS %s \n", myName.IP);
                myName.port = getPort(sockfd);
                printf("SERVER_PORT %d \n", myName.port);
                return 1;
            }
        }
    }
    
    return 0;
}
    //printf("server waiting for connections! \n");
    //freeaddrinfo(serverInfo);
    //sa.sa_handler = sigchld_handler;
    //sigemptyset(&sa.sa_mask);
    //sa.sa_flags = SA_RESTART;
    //if (sigaction(SIGCHLD, &sa, NULL) == -1)
    //{
    //    perror("sigaction");
    //    return 0;
    //}

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


int rpcInit(void)
{
    if(openConnBinder())
        printf("Connection to Binder successful\n");
    if(listen())
        printf("Server listening\n");
    return 1;
}

int rpcRegister(char *name, int *argTypes, skeleton f)
{
    message msg;
    msg = createRegMsg(myName.IP, myName.port, name, argTypes);
    void *rcvdMsg = sendRecvBinder(bindSockfd, msg);
    free(msg);
    if(rcvdMsg == 0)
        printf("REgistration on binder failed\n");
    else
    {
        skeleArgs *key;
        switch(((termMsg*)rcvdMsg)->type)
        {
            case REGISTER_SUCCESS:
                key = createFuncArgs(name, argTypes);
                serverStore[*key] = f;
                break;
            case REGISTER_FAILURE:
                printf("REgistration on binder failed\n");
        }
    }
    free(rcvdMsg);
    return 1; 
}


int rpcExecute(void)
{
   fd_set master;
   int maxfd, currfd;
   FD_ZERO(&master);
   FD_SET(sockfd, &master);
   maxfd = sockfd;
   terminate = false;

    while(!terminate)
    {
        //setsockopt(currfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)); (this will make currfd reusable for multiple incoming connections)
        if(select(maxfd + 1, &master, NULL, NULL, NULL) == -1) {
            perror("rpcExecute: Select failed");
            continue;
        }
        for(currfd = 0; currfd <= maxfd; currfd++)
        {
            if(FD_ISSET(currfd, &master))
            {
                if(currfd == sockfd)
                {
                    currfd = acceptSocket(sockfd);
                    if(currfd >= 0)
                    {
                        FD_SET(currfd, &master);
                        if(currfd > maxfd)
                        {
                            maxfd = currfd;
                        }
                    }
                    else
                    {
                        perror("Server accepting error");
                    }
                }
                else
                {
                    if(handleIncomingConn(currfd) <= 0) {
                        close(currfd);
                        perror("Request handling error");
                    }
                    FD_CLR(currfd, &master);
                }
            }
        }

    }
    return 1;
}

