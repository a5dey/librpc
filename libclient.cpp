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
std::vector<Server*> *serverStore;

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

struct func {
    char *name;
    int *argTypes;
};

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

int insertIntoCache(locSucMsg *msg, func *functions)
{
    skeleArgs *key;
    std::vector<Server*>::iterator itServer;
    key = createFuncArgs(functions->name, functions->argTypes);
    location *value;
    value = createLocation(msg->IP, msg->port);
    std::pair<std::set<skeleArgs*, cmp_skeleArgs>::iterator,bool> ret;
    if(key != 0 && value != 0)
    {
        for(itServer = (*serverStore).begin(); itServer != (*serverStore).end(); itServer++)
        {
            if(compare(*(*itServer)->loc, *value))
            {
                (*((*itServer)->functions)).insert(key);
                break;
            }
        }
        if(itServer == (*serverStore).end())
        {
            Server *newServer = new Server;
            newServer->loc = value;
            newServer->functions = new std::set<skeleArgs*, cmp_skeleArgs>;
            ret = (*(newServer->functions)).insert(key);
            (*serverStore).insert((*serverStore).begin(), newServer);
        }
    }
    return 1;
}

int retrieveFromCache(func *funct)
{
    skeleArgs *key;
    key = createFuncArgs(funct->name, funct->argTypes);
    location *value = NULL;
    std::vector<Server*>::iterator itServer;
    std::set<skeleArgs*, cmp_skeleArgs>::iterator itFuncs;
    int found = 0;
    int j;
    if(key != 0)
    {
        for(itServer = (*serverStore).begin(), j = 0; itServer != (*serverStore).end(); itServer++, j++)
        {
            itFuncs = (*(*itServer)->functions).find(key);
            if(itFuncs != (*(*itServer)->functions).end())
            {
                found = 1;
                value[j] = *(*itServer)->loc;
                printf("Server %d:", j);
            }
        }
    }
    return found;
}

int rpcCall(char *name, int *argTypes, void **args)
{
    message exec_msg;
    openConnBinder();
    message msg;
    msg = createLocReqMsg(LOC_REQUEST, name, argTypes);
    assert( msg != NULL);
    void *rcvdMsg = sendRecvBinder(bindSockfd, msg);
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
                return sendExecuteToServer((locSucMsg*)rcvdMsg, exec_msg, args, argTypes);
            case LOC_FAILURE:
                return -1;
        }
    }
    return 1;
}

int rpcCacheCall(char * name, int * argTypes, void ** args)
{
    openConnBinder();
    func *functions = NULL;
    size_t length = 0;
    location *loc;
    message m, msg, rcvdMsg = NULL;
    functions->name = name;
    functions->argTypes = argTypes;
    int found = retrieveFromCache(functions);
    if (found == 0)
    {
        msg = createLocReqMsg(LOC_CACHE_REQUEST, name, argTypes);
        send(bindSockfd, msg, getLengthOfMsg(msg), 0);
        while(1)
        {
            if(int n = recv(bindSockfd, rcvdMsg, getLengthOfMsg(rcvdMsg), 0) > 0)
            {
                termMsg *t = new termMsg;
                m = rcvdMsg;
                m = (message)convFromByte(rcvdMsg, &t->type, TYPE_SIZE);
                switch(t->type)
                {
                    case LOC_CACHE_SUCCESS:
                    {
                        memcpy(&length, rcvdMsg, DATALEN_SIZE);
                        locSucMsg *rc = parseLocSucMsg(LOC_CACHE_SUCCESS, rcvdMsg, getLengthOfMsg(rcvdMsg));
                        insertIntoCache(rc, functions);
                        break;
                    }
                    case LOC_CACHE_FAILURE:
                        printf("Server could not be located\n");
                }
            }
        }
    }
    return 1;
}

int rpcTerminate()
{
    message msg;
    openConnBinder();
    msg = createTermMsg(TERMINATE);
    sendRecvBinder(bindSockfd, msg);
    return 1;
}
