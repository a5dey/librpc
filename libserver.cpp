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
#include "rpc.h"
#include <sys/types.h>
#include <sys/time.h>
#include <map>

#define NUM_THREADS 10000

static int sockfd;
static int bindSockfd;

static addrInfo myName;
static addrinfo *myInfo;
static addrinfo *binderInfo;
static std::map<skeleArgs*, skeleton, cmp_skeleArgs> store;

static bool terminate;

struct thread_data {
    int _sockfd;
    std::map<skeleArgs*, skeleton, cmp_skeleArgs> *funcStore;
};

void printServerStore()
{
    printf("Printing Function Store\n");
    std::map<skeleArgs*, skeleton, cmp_skeleArgs>::iterator it;
    printf("Functions registered: \n");
    for(it = store.begin(); it != store.end(); it++)
    {
            printf("Name: %s\n", it->first->name);
    }
}

int compare(skeleArgs *a, skeleArgs *b)
{
    int compName = strcmp(a->name, b->name);
    int compArgTypes = 0;
    int i = 0;
    while(1)
    {
        if(a->argTypes[i] < b->argTypes[i])
        {
            compArgTypes = -1;
            break;
        }
        else if (a->argTypes[i] > b->argTypes[i])
        {
            compArgTypes = 1;
            break;
        }
        if(a->argTypes[i] == 0 || b->argTypes[i] == 0)
            break;
        i++;
    }
    return (compName == 0 && compArgTypes == 0)?1:0;
}

int handleExecute(exeMsg *msg, int _sockfd, struct thread_data *arg)
{
    std::map<skeleArgs*, skeleton, cmp_skeleArgs> *funcStore = arg->funcStore;
    skeleArgs *key;
    message byteMsgSent;
    key = createFuncArgs(msg->name, msg->argTypes);
    std::map<skeleArgs*, skeleton, cmp_skeleArgs>::iterator it;
    printServerStore();
    int (*func)(int *argTypes, void **args) = NULL;
    int reason;
    for(it = (*funcStore).begin(); it != (*funcStore).end(); it++)
    {
        if(compare(it->first, key))
        {
            func = it->second;
            reason = func(msg->argTypes, msg->args);
            break;
        }
    }
    if(it == (*funcStore).end())
        reason = -1;
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
    if(_sockfd == bindSockfd)
    {
        close(sockfd);
        close(bindSockfd);
    }
    exit(0);
}

void* handleIncomingConn(void *threadArg)
{
    struct thread_data *arg = (struct thread_data *)threadArg;
    int _sockfd = 0;
    assert(arg->_sockfd != NULL);
    memcpy(&_sockfd, &(arg->_sockfd), INT_SIZE);
    void* rcvdMsg = recvFromEntity(_sockfd);
    message retMsg;
    switch(((termMsg*)rcvdMsg)->type)
    {
        case EXECUTE:
            handleExecute((exeMsg*)rcvdMsg, _sockfd, arg);
            close(_sockfd);
            pthread_exit((void *)1);
        case TERMINATE:
            handleTerminate(_sockfd);
            close(_sockfd);
            pthread_exit((void *)1);
        default:
            retMsg = createTermMsg(MESSAGE_INVALID);
    }
    if(sendToEntity(_sockfd, retMsg) < 0)
    {
        perror("Reply to EXECUTE failed");
        close(_sockfd);
        pthread_exit((void *)1);
    }
    close(_sockfd);
    pthread_exit((void *)1);
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


int rpcInit()
{
    if(openConnBinder())
        printf("Connection to Binder successful\n");
    if(listen())
        printf("Server listening\n");
    return 1;
}

int rpcRegister(char *name, int *argTypes, skeleton f)
{
    assert(name != NULL);
    assert(argTypes != NULL);
    assert(f != NULL);
    assert(myName.IP != NULL);
    assert(myName.port != NULL);
    message msg;
    msg = createRegMsg(myName.IP, myName.port, name, argTypes);
    assert(msg != NULL);
    assert(bindSockfd != NULL);
    void *rcvdMsg = sendRecvBinder(bindSockfd, msg);
    assert(rcvdMsg != NULL);
    if(rcvdMsg == 0)
        printf("REgistration on binder failed\n");
    else
    {
        skeleArgs *key;
        switch(((termMsg*)rcvdMsg)->type)
        {
            case REGISTER_SUCCESS:
                key = createFuncArgs(name, argTypes);
                store[key] = f;
                break;
            case REGISTER_FAILURE:
                printf("REgistration on binder failed\n");
        }
    }
    free(rcvdMsg);
    return 1; 
}


int rpcExecute()
{
    int status;
    void *rc;
    pthread_t threads[NUM_THREADS];
    struct thread_data threadDataArr[NUM_THREADS];
    int num = 0;
    //terminate = false;
    //while(!terminate)
    //{
    //    int newSockfd;
    //    if((newSockfd = acceptSocket(sockfd)) > 0)
    //    {
    //        threadDataArr[num].funcStore = &store;
    //        threadDataArr[num]._sockfd = newSockfd;
    //        status = pthread_create(&threads[num], NULL, handleIncomingConn, (void *)&threadDataArr[num]);
    //        num++;
    //    }
    //}
    //for(int t=0; t<num; t++) 
    //    pthread_join(threads[t], &rc);
    fd_set master;
    int maxfd, currfd, newfd;
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
                    newfd = acceptSocket(sockfd);
                    if(newfd >= 0)
                    {
                        FD_SET(newfd, &master);
                        if(newfd > maxfd)
                        {
                            maxfd = newfd;
                        }
                    }
                    else
                    {
                        perror("Server accepting error");
                    }
                }
                else
                {
                    threadDataArr[num].funcStore = &store;
                    threadDataArr[num]._sockfd = currfd;
                    status = pthread_create(&threads[num], NULL, handleIncomingConn, (void *)&threadDataArr[num]);
                    num++;
                    //if(handleIncomingConn(currfd) <= 0) {
                    //    close(currfd);
                    //    perror("Request handling error");
                    //}
                    FD_CLR(currfd, &master);
                }
            }
        }

    }

    return 1;
}
