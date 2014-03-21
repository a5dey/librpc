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
#include <iostream>
#include <string>
#include "network.h"

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void *get_in_addr(struct sockaddr *sa)
{
    if(sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

addrinfo* getAddrInfo(char *IP, char *port)
{   
    struct addrinfo *entityInfo;
    struct addrinfo base;
    int status;
    memset(&base, 0, sizeof base);
    base.ai_family = AF_UNSPEC;
    base.ai_socktype = SOCK_STREAM;
    base.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(IP, port, &base, &entityInfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
    return entityInfo;
}

int getSocket()
{
    int _sockfd;
    if((_sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket error");
        return -1;
    }
    return _sockfd;
}

int bindSocket(int sockfd, struct addrinfo *myInfo)
{
    struct addrinfo *p;
    for(p = myInfo; p != NULL; p = p->ai_next)
    {
        if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("bind error");
                continue;
        }
        break;
    }
    if (p == NULL)
    {
        fprintf(stderr, "failed to bind\n");
        return 0;
    }
    return sockfd;
}

int listenSocket(int sockfd)
{
    if(listen(sockfd, BACKLOG) == -1) 
    {
        perror("listen error");
        return 0;
    }
    return 1;
}

char* getMyIP()
{
    char *hostname = new char[HOSTNAME_LEN];
    printf("Came to get name\n");
    if(gethostname(hostname, HOSTNAME_LEN))
    {
        perror("Error: can't get host name.");
        return 0;
    }
    return hostname;
}

int getPort(int _sockfd)
{
    struct sockaddr_in sin;
    socklen_t addrSize;
    addrSize = sizeof sin;
    if(getsockname(_sockfd, (struct sockaddr *)&sin, &addrSize) == -1)
    {
        perror("getsockname error");
        return 0;
    }
    return ntohs(sin.sin_port);
}

int connectSocket(int _sockfd, struct addrinfo *entityInfo)
{
    struct addrinfo *p;
    for(p = entityInfo; p != NULL; p = p->ai_next)
    {
        if(connect(_sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(_sockfd);
            return CONNECTION_CLOSED;
        }
        break;
    }
    if (p == NULL)
    {
        fprintf(stderr, "failed to connect\n");
        return 0;
    }
    return _sockfd;
}

int acceptSocket(int _sockfd)
{
    struct sockaddr_storage clAddr;
    socklen_t addrSize = sizeof clAddr;
    char clName[INET_ADDRSTRLEN];
    int newSockfd = accept(_sockfd, (struct sockaddr *)&clAddr, &addrSize);
    if(newSockfd == -1)
    {
        return CONNECTION_CLOSED;
    }
    return newSockfd;
}

void* recvFromEntity(int _sockfd)
{
    //Returns received message, if error returns -1
    assert(_sockfd != NULL);
    int numbytes;
    message rcvdLenMsg = allocMemMsg(DATALEN_SIZE);
    if((numbytes = recv(_sockfd, rcvdLenMsg, DATALEN_SIZE, 0)) <= 0)
    {
        return 0;
    }
    assert(rcvdLenMsg != NULL);
    size_t dataLen = getLengthOfMsg(rcvdLenMsg);
    assert(dataLen != NULL);
    printf("Size of datalen is %zu\n", dataLen);
    dataLen -= DATALEN_SIZE;
    message rcvdMsg = allocMemMsg(dataLen);
    assert(dataLen != NULL);
    byte *data = rcvdMsg;
    if((numbytes = recv(_sockfd, data, dataLen, 0)) <= 0)
    {
        return 0;
    }
    printf("Received Num bytes: %d on %d\n", numbytes, _sockfd);
    assert(rcvdMsg != NULL);
    void *prsdMsg = parseMsg(rcvdMsg, dataLen);
    assert(prsdMsg != NULL);
    free(rcvdLenMsg);
    free(rcvdMsg);
    return prsdMsg;
}

void* sendRecvBinder(int _sockfd, message msg)
{
    assert(_sockfd != NULL);
    assert(msg != NULL);
    void* rcvdMsg;
    while(1)
    {
        if(sendToEntity(_sockfd, msg) == 0)
            return 0;
        else
            rcvdMsg = recvFromEntity(_sockfd);
        if(rcvdMsg != 0 && ((termMsg*)rcvdMsg)->type != SEND_AGAIN)
            break;
        else
            return 0;
    }
    return rcvdMsg;
}

int sendToEntity(int _sockfd, message msg)
{
    //Return 1 if send successful, return -1 otherwise.
    assert(_sockfd != NULL);
    assert(msg != NULL);
    int numbytes;
    size_t length = getLengthOfMsg(msg);
    assert(length != NULL);
    if ((numbytes = send(_sockfd, msg, length, 0)) == -1) 
    {
        return 0;
    }
    assert(msg != NULL);
    free(msg);
    printf("Sent Num bytes: %d on %d\n", numbytes, _sockfd);
    return 1;

}


