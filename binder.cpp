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

#define PORT "0"
#define BACKLOG 20
#define MAXDATASIZE 10000
#define REGISTER            1
#define REGISTER_SUCCESS    2
#define REGISTER_FAILURE    3
#define LOC_REQUEST         4
#define LOC_SUCCESS         5
#define LOC_FAILURE         6
#define EXECUTE             7
#define EXECUTE_SUCCESS     8
#define EXECUTE_FAILURE     9
#define TERMINATE           10

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
    char buf[MAXDATASIZE];
    int numbytes;
    if((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1)
    {
        perror("Error in receiving");
        return 2;
    }
    buf[numbytes] = '\0';
    int sizeLen = 4;
    int typeLen = 4;
    int msgSize = 0;
    for (int j = 0; j < sizeLen; j++)
    {
        msgSize += (buf[j] << (sizeLen-1-j));
    }
    int type = buf[sizeLen+typeLen-1];
    switch(type) {
        case 1: registerServ(sockfd, msgSize, buf+sizeLen+typeLen);
        break;
        case 4: locRequest(sockfd, msgSize, buf+sizeLen+typeLen);
                break;
        case 10: terminate(sockfd);
                 break;
        default: printf("Message invalid\n");
    }
    return 1;
}

int main(void)
{
    int sockfd, newSockfd;
    struct addrinfo base, *binderInfo, *p;
    int status;
    struct sockaddr_storage clAddr;
    socklen_t addrSize, hostAddrSize;
    char hostname[INET_ADDRSTRLEN];
    char clName[INET_ADDRSTRLEN];
    int yes = 1;
    struct sigaction sa;
    //char *hostname;
    size_t size;
    struct sockaddr_in sin;

    memset(&base, 0, sizeof base);
    base.ai_family = AF_UNSPEC;
    base.ai_socktype = SOCK_STREAM;
    base.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, PORT, &base, &binderInfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return 1;
    }

    for(p = binderInfo; p != NULL; p = p->ai_next)
    {
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("binder: socket error");
                continue;
        }
        if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("binder: bind error");
                continue;
        }
        break;
    }
    if (p == NULL)
    {
        fprintf(stderr, "binder failed to bind\n");
        return 2;
    }

    freeaddrinfo(binderInfo);

    if(listen(sockfd, BACKLOG) == -1) 
    {
        perror("listen error");
            exit(1);
    }

    hostAddrSize = sizeof sin;
    if(getsockname(sockfd, (struct sockaddr *)&sin, &hostAddrSize) == -1)
    {
        perror("getsockname error");
    }
    else
    {
        inet_ntop(AF_INET, &(sin.sin_addr), hostname, INET_ADDRSTRLEN);
        printf("BINDER_ADDRESS %s \n", hostname);
        printf("BINDER_PORT %d \n", ntohs(sin.sin_port));
    }

    //printf("binder waiting for connections! \n");

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    while(1)
    {
        addrSize = sizeof clAddr;
        newSockfd = accept(sockfd, (struct sockaddr *)&clAddr, &addrSize);
        if(newSockfd == -1)
        {
            perror("error on accept");
                continue;
        }

        inet_ntop(clAddr.ss_family, get_in_addr((struct sockaddr *)&clAddr), clName, sizeof clName);
        //printf("binder: got connection from %s. \n", clName);

        if(!fork()) 
        {
            close(sockfd);
            handleIncomingConn(newSockfd);
            close(newSockfd);
            exit(0);
        }
        close(newSockfd);
    }
    return 0;
}


