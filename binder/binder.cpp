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
#include <map>
#include <vector>
#include "../network/network.h"

//#define PORTY "10000"
struct cmp_skeleArgs{
    bool operator()(skeleArgs a, skeleArgs b)
    {
        return strcmp(a.name, b.name) < 0;
    }
};

static int sockfd;

static addrInfo myName;
static addrinfo *myInfo;
static std::map<skeleArgs, location, cmp_skeleArgs> binderStore;
static std::vector<int> serverList;


int handleRegister(regMsg *msg, int _sockfd)
{
    printf("Server identifier %s, port %d, name of function %s\n", msg->IP, msg->port, msg->name);
    skeleArgs *key;
    location *value;
    message byteMsgSent;
    int reason;
    key = createFuncArgs(msg->name, msg->argTypes);
    value = createLocation(msg->IP, msg->port);
    std::map<skeleArgs, location, cmp_skeleArgs>::iterator it;
    if(key != 0 && value != 0)
    {
        printf("Registering function\n");
        it = binderStore.find(*key);
        if(it == binderStore.end())
        {
            binderStore[*key] = *value;
            reason = 1;
        }
        else
            reason = 2;
        printf("Registration successful\n");
        byteMsgSent = createSucFailMsg(REGISTER_SUCCESS, reason);
    }
    else
    {
        reason = -1;
        byteMsgSent = createSucFailMsg(REGISTER_FAILURE, reason);
    }
    size_t dataLen;
    convFromByte(byteMsgSent, &dataLen, DATALEN_SIZE);
    printf("Size of datalen is %d\n", dataLen);
    if(sendToEntity(_sockfd, byteMsgSent) == 0)
    {
        perror("Reply to REGISTER failed");
        return -1;
    }
    return 1;
}

int handleLocationRequest(locReqMsg *msg, int _sockfd)
{
    return 1;
}

int handleTerminate(int _sockfd)
{
    return 1;
}

int handleIncomingConn(int _sockfd)
{
    void* rcvdMsg;
    message retMsg;
    if((rcvdMsg = recvFromEntity(_sockfd)) != 0)
    {
        switch(((termMsg*)rcvdMsg)->type)
        {
            case REGISTER:
                return handleRegister((regMsg*)rcvdMsg, _sockfd);
            case LOC_REQUEST:
                return handleLocationRequest((locReqMsg*)rcvdMsg, _sockfd);
            case TERMINATE:
                return handleTerminate(_sockfd);
            default:
                retMsg = createTermMsg(MESSAGE_INVALID);
        }
    }
    else
        retMsg = createTermMsg(SEND_AGAIN);
    if(sendToEntity(_sockfd, retMsg) < 0)
    {
        perror("Reply to request failed");
        return -1;
    }
    return 1;
}


    //rcvdBytes = numbytes;
    //while(rcvdBytes <= len)
    //{
    //    printf("Entering loop\n");
    //    if((numbytes = recv(sockfd, (void*)dat+rcvdBytes, MAXDATA_SIZE, 0)) == -1)
    //    {
    //        perror("Error in receiving");
    //        return 2;
    //    }
    //    rcvdBytes += numbytes;
    //}
    //}
    //int type = buf[sizeLen+typeLen-1];

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


