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

//Ankita please modify the definitions below. Also use the structs defined in
//message.h and if you want to create your own here make sure they have
//a different name from the one defined in message.h
//struct cmp_skeleArgsNoPointer{
//    bool operator()(skeleArgs a, skeleArgs b)
//    {
//        int compName = strcmp(a.name, b.name);
//        int compArgTypes = 0;
//        int i = 0;
//        while(1)
//        {
//            if(a.argTypes[i] < b.argTypes[i])
//            {
//                compArgTypes = -1;
//                break;
//            }
//            else if (a.argTypes[i] > b.argTypes[i])
//            {
//                compArgTypes = 1;
//                break;
//            }
//            if(a.argTypes[i] == 0 || b.argTypes[i] == 0)
//                break;
//            i++;
//        }
//        return compName < 0 || (compName == 0 && compArgTypes < 0);
//    }
//};
//
//struct func {
//    char *name;
//    int *argTypes;
//};
//struct Server {
//    location loc;
//    std::set<skeleArgs, cmp_skeleArgsNoPointer> functions;
//};

static std::vector<Server> cachedServerList;

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

//int compare(location a, location b)
//{
//    if (strcmp(a.IP, b.IP) == 0 && a.port == b.port)
//        return 1;
//    else
//        return 0;
//}
//
//int insertIntoCache(void *clientMsg, func *functions)
//{
//    skeleArgs *key;
//    std::vector<Server>::iterator it;
//    location *value;
//    locSucMsg *msg = (locSucMsg *)clientMsg;
//    value = createLocation(msg->IP, msg->port);
//    for(it = cachedServerList.begin(); it != cachedServerList.end(); it++)
//    {
//        if(compare(it->loc, *value))
//        {
//            key = createFuncArgs(functions->name, functions->argTypes);
//            (it->functions).insert(*key);
//            break;
//        }
//    }
//    if(it == cachedServerList.end())
//    {
//        Server *newServer = new Server;
//        newServer->loc = *value;
//        key = createFuncArgs(functions->name, functions->argTypes);
//        newServer->functions.insert(*key);
//        cachedServerList.insert(cachedServerList.begin(), *newServer);
//    }
//    return 1;
//}
//
//location *retrieveFromCache(func *funct)
//{
//    skeleArgs *key;
//    key = createFuncArgs(funct->name, funct->argTypes);
//    std::vector<Server>::iterator it;
//    location *value = NULL;
//    std::set<skeleArgs, cmp_skeleArgsNoPointer>::iterator itFuncs;
//    for(it = cachedServerList.begin(); it != cachedServerList.end(); it++)
//    {
//        it->functions.find(*key);
//        if(itFuncs != it->functions.end())
//        {
//            *value = it->loc;
//            break;
//        }
//    }
//    return value;
//}

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

int rpcCacheCall(char * name, int * argTypes, void ** args)
{
//    openConnBinder();
//    func *functions;
//    location *loc;
//    message m, msg, rcvdMsg;
//    functions->name = name;
//    functions->argTypes = argTypes;
//    loc = retrieveFromCache(functions);
//    if (loc == NULL)
//    {
//        msg = createLocReqMsg(LOC_CACHE_REQUEST, name, argTypes);
//        send(bindSockfd, msg, getLengthOfMsg(msg), 0);
//        while(1)
//        {
//            if(int n = recv(bindSockfd, rcvdMsg, getLengthOfMsg(rcvdMsg), 0) > 0)
//            {
//                termMsg *t = new termMsg;
//                m = rcvdMsg;
//                m = (message)convFromByte(rcvdMsg, &t->type, TYPE_SIZE);
//                switch(t->type)
//                {
//                    case LOC_CACHE_SUCCESS:
//                    {
//                        void *rcvd = (void *)parseLocSucMsg(LOC_CACHE_SUCCESS, rcvdMsg, getLengthOfMsg(rcvdMsg));
//                        insertIntoCache(rcvd, functions);
//                        break;
//                    }
//                    case LOC_CACHE_FAILURE:
//                        printf("Server could not be located\n");
//                }
//            }
//        }
//    }
//    //call to server
    return 1;
}

int rpcTerminate(void)
{
    message msg;
    openConnBinder();
    msg = createTermMsg(TERMINATE);
    sendRecvBinder(bindSockfd, msg);
    return 1;
}
