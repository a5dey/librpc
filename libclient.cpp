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

static int bindSockfd;
std::vector<Server*> *serverStore;

int moveToArgs(exeMsg *msg, void **args, int *argTypes)
{
    size_t argTypesLen = getArgTypesLen(argTypes);
    int numArgs = (argTypesLen/INT_SIZE) - 1;
    size_t lengths[numArgs];
    int lenArray = 0;
    int dataType;
    for(int i = 0; i < numArgs; i++)
    {
        lenArray = argTypes[i] & 0xffff;
        if(lenArray == 0)
            lenArray = 1;
        dataType = (argTypes[i] >> 16) & 0xff;
        lengths[i] = getDataTypeLen(dataType)*lenArray;
    }
    for(int i = 0; i < numArgs; i++)
    {
        memcpy(args[i], msg->args[i], lengths[i]);
    }
    return 1;
}

int sendExecuteToServer(locSucMsg *serverLoc, message msg, void **args, int *argTypes)
{
    int servSockfd;
    void *rcvdMsg;
    struct addrinfo *serverInfo;
    char *IP = serverLoc->IP;
    int p = serverLoc->port;
    char *port = (char*)malloc(INT_SIZE);
    std::stringstream out;
    out << p;
    strcpy(port, out.str().c_str());
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
                    moveToArgs((exeMsg*)rcvdMsg, args, argTypes);
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

int insertIntoCache(locSucMsg *msg, skeleArgs *functions)
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

location *retrieveFromCache(skeleArgs *funct)
{
    skeleArgs *key;
    key = createFuncArgs(funct->name, funct->argTypes);
    location *value = '\0';
    std::vector<Server*>::iterator itServer;
    std::set<skeleArgs*, cmp_skeleArgs>::iterator itFuncs;
    if(key != 0)
    {
        for(itServer = (*serverStore).begin(); itServer != (*serverStore).end(); itServer++)
        {
            itFuncs = (*(*itServer)->functions).find(key);
            if(itFuncs != (*(*itServer)->functions).end())
            {
                value = (*itServer)->loc;
                (*serverStore).push_back(*itServer);
                break;
            }
        }
    }
    return value;
}

int rpcCall(char *name, int *argTypes, void **args)
{
    message exec_msg;
    openConnBinder();
    message msg;
    msg = createLocReqMsg(LOC_REQUEST, name, argTypes);
    assert( msg != NULL);
    void *rcvdMsg = sendRecvBinder(bindSockfd, msg);
    if(rcvdMsg == 0)
    {
        printf("Location Request failed\n");
        return BINDER_NOT_FOUND;
    }
    else
    {
        switch(((termMsg*)rcvdMsg)->type)
        {
            case LOC_SUCCESS:
                printf("LOCATION REQUEST SUCCESS\n");
                exec_msg = createExeSucMsg(EXECUTE, name, argTypes, args);
                return sendExecuteToServer((locSucMsg*)rcvdMsg, exec_msg, args, argTypes);
            case LOC_FAILURE:
                return ((sucFailMsg*)rcvdMsg)->reason;
        }
    }
    return 1;
}

int rpcCacheCall(char * name, int * argTypes, void ** args)
{
    openConnBinder();
    skeleArgs *functions;
    location *loc;
    message m, msg, rcvdMsg, exec_msg;
    functions = createFuncArgs(name, argTypes);
    assert(functions != NULL);
    rcvdMsg = allocMemMsg(DATALEN_SIZE);
    loc = retrieveFromCache(functions);
    if (loc == '\0')
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
                        locSucMsg *rc = parseLocSucMsg(LOC_CACHE_SUCCESS, rcvdMsg, getLengthOfMsg(rcvdMsg));
                        insertIntoCache(rc, functions);
                        exec_msg = createExeSucMsg(EXECUTE, name, argTypes, args);
                        return sendExecuteToServer(rc, exec_msg, args, argTypes);
                        break;
                    }
                    case LOC_CACHE_FAILURE:
                        printf("Server could not be located\n");
                        break;
                }
            }
        }
    }
    else
    {
        locSucMsg *rc = new locSucMsg;
        rc->type = LOC_CACHE_SUCCESS;
        rc->IP = loc->IP;
        rc->port = loc->port;
        exec_msg = createExeSucMsg(EXECUTE, name, argTypes, args);
        sendExecuteToServer(rc, exec_msg, args, argTypes);
        
    }
    return 1;
}

int rpcTerminate()
{
    message msg;
    openConnBinder();
    msg = createTermMsg(TERMINATE);
    sendToEntity(bindSockfd, msg);
    return 1;
}
