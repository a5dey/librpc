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
#include "message/message.cpp"

static int sockfd;
static int bindSockfd;

struct cmp_skeleArgs{
    bool operator()(skeleArgs a, skeleArgs b)
    {
        int compName = strcmp(a.name, b.name);
        int compArgTypes = 0;
        int i = 0;
        while(1)
        {
            if(a.argTypes[i] < b.argTypes[i])
            {
                compArgTypes = -1;
                break;
            }
            else if (a.argTypes[i] > b.argTypes[i])
            {
                compArgTypes = 1;
                break;
            }
            if(a.argTypes[i] == 0 || b.argTypes[i] == 0)
                break;
            i++;
        }
        return compName < 0 || (compName == 0 && compArgTypes < 0);
    }
};

struct func {
    char *name;
    int *argTypes;
};
struct Server {
    location loc;
    std::set<skeleArgs, cmp_skeleArgs> functions;
};

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

int compare(location a, location b)
{
    if (strcmp(a.IP, b.IP) == 0 && a.port == b.port)
        return 1;
    else
        return 0;
}

int insertIntoCache(void *clientMsg, func *functions)
{
    skeleArgs *key;
    std::vector<Server>::iterator it;
    location *value;
    locSucMsg *msg = (locSucMsg *)clientMsg;
    value = createLocation(msg->IP, msg->port);
    for(it = cachedServerList.begin(); it != cachedServerList.end(); it++)
    {
        if(compare(it->loc, *value))
        {
            key = createFuncArgs(functions->name, functions->argTypes);
            (it->functions).insert(*key);
            break;
        }
    }
    if(it == cachedServerList.end())
    {
        Server *newServer = new Server;
        newServer->loc = *value;
        key = createFuncArgs(functions->name, functions->argTypes);
        newServer->functions.insert(*key);
        cachedServerList.insert(cachedServerList.begin(), *newServer);
    }
    return 1;
}

location *retrieveFromCache(func *funct)
{
    skeleArgs *key;
    key = createFuncArgs(funct->name, funct->argTypes);
    std::vector<Server>::iterator it;
    location *value = NULL;
    std::set<skeleArgs, cmp_skeleArgs>::iterator itFuncs;
    for(it = cachedServerList.begin(); it != cachedServerList.end(); it++)
    {
        it->functions.find(*key);
        if(itFuncs != it->functions.end())
        {
            *value = it->loc;
            break;
        }
    }
    return value;
}

int rpcCall(char *name, int *argTypes, void **args)
{
    char *IP;
    int port;
    message exec_msg;
    struct addrinfo *serverInfo;
    openConnBinder();
    message msg;
    msg = createLocReqMsg(LOC_REQUEST, name, argTypes);
    void *clientMsg = sendRecvBinder(bindSockfd, msg);//sendRecvBinder needs to call createbndrMsg in message.cpp for recvFromEntity
    void *prsdMsg = parseMsg((message)clientMsg, getLengthOfMsg((message)clientMsg));//binder will set the type to LOC_SUCCESS or LOC_FAILURE
    
    locSucMsg *m = (locSucMsg *)prsdMsg;
    if (m->type == LOC_SUCCESS)
    {
        IP = m->IP;
        port = m->port;
        message msg_exec = createExeSucMsg(EXECUTE, name, argTypes, args);
        serverInfo = getAddrInfo(IP, (char *)port);
        sockfd = getSocket();
        if(sockfd > 0)
        {
            if(connectSocket(sockfd, serverInfo) > 0)
            {
                sendToEntity(sockfd, msg_exec);
                exec_msg = (message)recvFromEntity(sockfd);
                void *parsed_execute = parseMsg(exec_msg, getLengthOfMsg(exec_msg));
            }
            else
            {
                EXIT_FAILURE;
                return -1;
            }
            return 0;
        }
    }
    return 1;
}

int rpcCacheCall(char * name, int * argTypes, void ** args)
{
    func *functions;
    location *loc;
    message msg;
    functions->name = name;
    functions->argTypes = argTypes;
    loc = retrieveFromCache(functions);
    if (loc == NULL)
    {
        msg = createLocReqMsg(LOC_REQUEST, name, argTypes);
        void *clientMsg = sendRecvBinder(bindSockfd, msg);
        switch(((termMsg*)clientMsg)->type)
        {
            case LOC_SUCCESS:
                insertIntoCache(clientMsg, functions);
                break;
            case LOC_FAILURE:
                printf("Server could not be located\n");
        }
    }
    //call to server
}

int rpcTerminate(void)
{
    message msg;
    openConnBinder();
    msg = createTermMsg(TERMINATE);
    sendRecvBinder(bindSockfd, msg);
    return 1;
}