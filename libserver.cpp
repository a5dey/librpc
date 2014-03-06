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
#include "message/message.h"



static int sockfd;
static int bindSockfd;


static addrInfo myName;
static addrinfo *myInfo;
static addrinfo *binderInfo;


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

//int rpcRegister(char *name, int *argTypes, skeleton f)
int rpcRegister()
{
    //createRegMsg(myName, name, argTypes);
    message *msg;
    msg = createMsg(myName.IP, myName.port);
    int numbytes;
    int length = sizeof(msg);
    printf("Server: Sent %s\n", msg);
    if ((numbytes = send(bindSockfd, msg, length, 0)) == -1) 
    {
        perror("Error in send");
        return -1;
    }
    printf("numbytes: %d\n", numbytes);
    //sendToBinder(bindSockfd, msg);
    return 1; 
}


int rpcExecute(void)
{
    while(1)
    {
    }
    return 1;
}

int main(void)
{
    rpcInit();
    rpcRegister();
    return 1;
}
