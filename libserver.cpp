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

typedef int (*skeleton)(int *, void **);

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

int handleIncomingConn(int sockfd)
{
    return 1;
}

int listen()
{

    int sockfd, newSockfd;
    struct addrinfo base, *serverInfo, *p;
    int status;
    struct sockaddr_storage clAddr;
    socklen_t addrSize, serverAddrSize;
    char servername[INET_ADDRSTRLEN];
    char clName[INET_ADDRSTRLEN];
    int yes = 1;
    struct sigaction sa;
    //char *servername;
    size_t size;
    struct sockaddr_in sin;

    memset(&base, 0, sizeof base);
    base.ai_family = AF_UNSPEC;
    base.ai_socktype = SOCK_STREAM;
    base.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, PORT, &base, &serverInfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return 0;
    }

    for(p = serverInfo; p != NULL; p = p->ai_next)
    {
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("server: socket error");
                continue;
        }
        if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: bind error");
                continue;
        }
        break;
    }
    if (p == NULL)
    {
        fprintf(stderr, "server failed to bind\n");
        return 0;
    }

    freeaddrinfo(serverInfo);

    if(listen(sockfd, BACKLOG) == -1) 
    {
        perror("listen error");
        return 0;
    }

    serverAddrSize = sizeof sin;
    if(getsockname(sockfd, (struct sockaddr *)&sin, &serverAddrSize) == -1)
    {
        perror("getsockname error");
        return 0;
    }
    else
    {
        inet_ntop(AF_INET, &(sin.sin_addr), servername, INET_ADDRSTRLEN);
        printf("SERVER_ADDRESS %s \n", servername);
        printf("SERVER_PORT %d \n", ntohs(sin.sin_port));
    }

    //printf("server waiting for connections! \n");

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        return 0;
    }
    return 1;
}

int openConnBinder() 
{
    struct addrinfo base, *binderInfo, *p;
    int status;
    char *buf;
    std::string str;
    //pthread_t threads[NUM_THREADS], readThread;
    //struct thread_data threadDataArr[NUM_THREADS];
    int sockfd;
    char name[INET_ADDRSTRLEN];
    int len;

    memset(&base, 0, sizeof base);
    base.ai_family = AF_UNSPEC;
    base.ai_socktype = SOCK_STREAM;

    char *binderIP = getenv("BINDER_ADDRESS");
    char *binderPort = getenv("BINDER_PORT");
    
    if ((status = getaddrinfo(binderIP, binderPort, &base, &binderInfo)) != 0)
    {
        fprintf(stderr, "client: getaddrinfo error: %s\n", gai_strerror(status));
        return 0;
    }
    for(p = binderInfo; p != NULL; p = p->ai_next)
    {
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("client: socket error");
                continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "client failed to connect\n");
        return 0;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)&p->ai_addr), name, sizeof name);
    
    return 1;


}


int rpcInit(void)
{
    return (listen() & openConnBinder());
}

int rpcRegister(char *name, int *argTypes, skeleton f)
{
   return 1; 
}


int rpcExecute(void)
{
    return 1;
}

int main(void)
{
    rpcInit();
    return 1;
}
