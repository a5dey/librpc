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
    int status;
    if(servSockfd > 0)
    {
        if((status = connectSocket(servSockfd, serverInfo)) > 0)
        {
            rcvdMsg = sendRecvBinder(servSockfd, msg);
        }
        else
        {
            return status;
        }
        if(rcvdMsg == 0)
        {
            printf("Location Request failed\n");
            return SERVER_NOT_FOUND;
        }
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
                    int rc = ((sucFailMsg*)rcvdMsg)->reason;
                    free(rcvdMsg);
                    return rc;
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


int rpcCall(char *name, int *argTypes, void **args)
{
    message exec_msg;
    openConnBinder();
    message msg;
    msg = createLocReqMsg(LOC_REQUEST, name, argTypes);
    assert( msg != NULL);
    void *rcvdMsg = sendRecvBinder(bindSockfd, msg);//sendRecvBinder needs to call createbndrMsg in message.cpp for recvFromEntity
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
                int rc = ((sucFailMsg*)rcvdMsg)->reason;
                free(rcvdMsg);
                return rc;
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

int rpcTerminate()
{
    message msg;
    openConnBinder();
    msg = createTermMsg(TERMINATE);
    sendToEntity(bindSockfd, msg);
    return 1;
}
