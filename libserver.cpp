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

int handleIncomingConn(int sockfd)
{
    return 1;
}

int listen()
{
    //char *servername;

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
    //pthread_t threads[NUM_THREADS], readThread;
    //struct thread_data threadDataArr[NUM_THREADS];
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
    message rcvdMsg = sendRecvBinder(bindSockfd, msg);
    //parseMsg(rcvdMsg);
    skeleArgs *key;
    key = createFuncArgs(name, argTypes);
    serverStore[*key] = f;
    //serverStore.insert(std::pair<skeleArgs, skeleton>(*key, f));
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

