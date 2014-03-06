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
#include "../network/network.h"
#include "../message/message.h"


static int sockfd;

static addrInfo myName;
static addrinfo *myInfo;

int registerServ(int sockfd, int len, char *msg)
{
    printf("registration successful\n");
    return 1;
}

int locRequest(int sockfd, int len, char *msg)
{
    return 1;
}

int terminate(int sockfd)
{
    return 1;
}

int handleIncomingConn(int sockfd)
{
    message *buf;
    int numbytes;
    if((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1)
    {
        perror("Error in receiving");
        return 2;
    }
    printf("numbytes: %d\n", numbytes);
    //buf[numbytes] = '\0';
    //int sizeLen = 4;
    //int typeLen = 4;
    //int msgSize = 0;
    //for (int j = 0; j < sizeLen; j++)
    //{
    //    msgSize += (buf[j] << (sizeLen-1-j));
    //}
    if(buf->head.type == REGISTER)
    {
        locSucMsg *dat;
        dat = (locSucMsg *)(buf->dat);
        printf("Binder: Received from server-->Message Type %s, IP %s, Port %d \n", buf->head.type, dat->IP, dat->port);
    }
    return 1;
    //int type = buf[sizeLen+typeLen-1];
    //switch(type) {
    //    case 1: registerServ(sockfd, msgSize, buf+sizeLen+typeLen);
    //    break;
    //    case 4: locRequest(sockfd, msgSize, buf+sizeLen+typeLen);
    //            break;
    //    case 10: terminate(sockfd);
    //             break;
    //    default: printf("Message invalid\n");
    //}
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
                printf("BINDER_ADDRESS %s \n", myName.IP);
                myName.port = getPort(sockfd);
                printf("BINDER_PORT %d \n", myName.port);
                
                return 1;
            }
        }
    }
    return 0;
}
    //printf("binder waiting for connections! \n");
int startAccept()
{
    printf("Starting to accept\n");
    while(1)
    {
        int newSockfd;
        if((newSockfd = acceptSocket(sockfd)) > 0)
        {
            if(!fork()) 
            {
                close(sockfd);
                handleIncomingConn(newSockfd);
                close(newSockfd);
                exit(0);
            }
            close(newSockfd);
        }
    }
    return 0;
}

int main(void)
{
    listen();
    startAccept();
    return 1;
}


