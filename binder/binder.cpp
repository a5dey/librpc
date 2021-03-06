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
#include <assert.h>
#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <set>
#include "../network/network.h"

#define NUM_THREADS 10000
//#define PORTY "10000"
static int sockfd;

static addrInfo myName;
static addrinfo *myInfo;
static std::set<int> *servList;

static std::vector<Server*> servStore;

static bool terminate;

struct thread_data {
    int _sockfd;
    std::vector<Server*> *serverStore;
    std::set<int> *serverList;
    bool *term;
};

void printServers(std::set<int> serverList)
{
    printf("Printing Servers\n");
    std::set<int>::iterator it;
    for (it = serverList.begin(); it != serverList.end(); it++)
        std::cout<< *it<<std::endl;
}


void printServerStore(std::vector<Server*> serverStore)
{
    printf("Printing Server Store\n");
    std::vector<Server*>::iterator it;
    std::set<skeleArgs*, cmp_skeleArgs>::iterator itfunc;
    for(it = serverStore.begin(); it != serverStore.end(); it++)
    {
        printf("Server Location: %s : %d\n", (*it)->loc->IP, (*it)->loc->port);
        printf("Functions registered: \n");
        //itfunc = (*((*it)->functions)).begin();
        for(itfunc = (*((*it)->functions)).begin(); itfunc != (*((*it)->functions)).end(); itfunc++)
        {
            printf("Name: %s, ArgTypes: ", (*itfunc)->name);
            for(int i = 0; (*itfunc)->argTypes[i] != 0; i++)
                printf("%d , ", (*itfunc)->argTypes[i]);
        }
    }
}

int compare(location a, location b)
{
    if (strcmp(a.IP, b.IP) == 0 && a.port == b.port)
        return 1;
    else 
        return 0;
}


void* handleRegister(regMsg *msg, struct thread_data *arg)
{

    std::vector<Server*> *serverStore = arg->serverStore;
    int _sockfd = arg->_sockfd;
    std::set<int> *serverList = arg->serverList;
    std::set<int>::iterator serverListIt;
    skeleArgs *key;
    location *value;
    message byteMsgSent;
    warning reason;
    key = createFuncArgs(msg->name, msg->argTypes);
    value = createLocation(msg->IP, msg->port);
    std::vector<Server*>::iterator it;
    std::pair<std::set<skeleArgs*, cmp_skeleArgs>::iterator,bool> ret;
    if(key != 0 && value != 0)
    {
        for(it = (*serverStore).begin(); it != (*serverStore).end(); it++)
        {
            if(compare(*((*it)->loc), *value))
            {
                ret = (*((*it)->functions)).insert(key);
                if(ret.second == true)
                    reason = OK;
                else
                    reason = FUNC_EXISTS;
                printf("Registration successful\n");
                break;
            }
        }
        if(it == (*serverStore).end())
        {
            Server *newServer = new Server;
            newServer->loc = value;
            newServer->functions = new std::set<skeleArgs*, cmp_skeleArgs>;
            ret = (*(newServer->functions)).insert(key);
            if(ret.second == true)
            (*serverStore).insert((*serverStore).begin(), newServer);
            reason = OK;
        }
        if((serverListIt = (*serverList).find(_sockfd)) == (*serverList).end())
            (*serverList).insert(_sockfd);
        //printServerStore(*serverStore);
        byteMsgSent = createSucFailMsg(REGISTER_SUCCESS, reason);
    }
    else
    {
        reason = INVALID_ARGS;
        byteMsgSent = createSucFailMsg(REGISTER_FAILURE, reason);
        value = 0;
    }
    size_t dataLen;
    convFromByte(byteMsgSent, &dataLen, DATALEN_SIZE);
    if(sendToEntity(_sockfd, byteMsgSent) == 0)
    {
        return (void*)0;
    }
    return (void*)value;
}

int handleDeregister(location *loc, struct thread_data *arg)
{
    int _sockfd = arg->_sockfd;
    std::vector<Server*> *serverStore = arg->serverStore;
    std::vector<Server*>::iterator it;
    std::set<int> *serverList = arg->serverList;
    std::set<int>::iterator serverListIt;
    for(it = (*serverStore).begin(); it != (*serverStore).end(); it++)
    {
        if(compare(*((*it)->loc), *loc))
        {
            (*serverStore).erase(it);
            break;
        }
    }
    return 1;
}

int handleLocationRequest(locReqMsg *msg, struct thread_data *arg)
{
    std::vector<Server*> *serverStore = arg->serverStore;
    int _sockfd = arg->_sockfd;
    skeleArgs *key;
    key = createFuncArgs(msg->name, msg->argTypes);
    location *value;
    message byteMsgSent;
    std::vector<Server*>::iterator itServer;
    std::set<skeleArgs*, cmp_skeleArgs>::iterator itFuncs;
    int found = 0;
    if(key != 0)
    {
        //printServerStore();
        for(itServer = (*serverStore).begin(); itServer != (*serverStore).end(); itServer++)
        {
            itFuncs = (*(*itServer)->functions).find(key);
            if(itFuncs != (*(*itServer)->functions).end())
            {
                found = 1;
                value = (*itServer)->loc;
                Server *pushServer = *itServer;
                (*serverStore).erase(itServer);
                (*serverStore).push_back(pushServer);
                byteMsgSent = createLocSucMsg(LOC_SUCCESS, value->IP, value->port);
                printf("Location Request successful\n");
                break;
            }
        }
        if(!found)
            byteMsgSent = createSucFailMsg(LOC_FAILURE, FUNC_NOT_FOUND);
    }
    else
            byteMsgSent = createSucFailMsg(LOC_FAILURE, INVALID_ARGS);
    if(sendToEntity(_sockfd, byteMsgSent) == 0)
    {
        perror("Reply to Location Request  failed");
        return -1;
    }
    return 1;
}

int handleCacheLocationRequest(locReqMsg *msg, struct thread_data *arg)
{
    std::vector<Server*> *serverStore = arg->serverStore;
    int _sockfd = arg->_sockfd;
    skeleArgs *key;
    key = createFuncArgs(msg->name, msg->argTypes);
    location *value;
    message byteMsgSent;
    std::vector<Server*>::iterator itServer;
    std::set<skeleArgs*, cmp_skeleArgs>::iterator itFuncs;
    int found =0;
    if(key != 0)
    {
        for(itServer = (*serverStore).begin(); itServer != (*serverStore).end(); itServer++)
        {
            itFuncs = (*(*itServer)->functions).find(key);
            if(itFuncs != (*(*itServer)->functions).end())
            {
                found =1;
                value = (*itServer)->loc;
                byteMsgSent = createLocSucMsg(LOC_CACHE_SUCCESS, value->IP, value->port);
                if(sendToEntity(_sockfd, byteMsgSent) == 0)
                {
                    perror("Reply to Location Request  failed");
                    return -1;
                }
            }
        }
        if(!found)
        {
            byteMsgSent = createSucFailMsg(LOC_CACHE_FAILURE, FUNC_NOT_FOUND);
            if(sendToEntity(_sockfd, byteMsgSent) == 0)
            {
                perror("Reply to Location Request  failed");
                return -1;
            }
        }
        else
        {
            byteMsgSent = createSucFailMsg(LOC_CACHE_FAILURE, END);
            if(sendToEntity(_sockfd, byteMsgSent) == 0)
            {
                perror("Reply to Location Request  failed");
                return -1;
            }
        }
    }
    return 1;
}

int handleTerminate(struct thread_data *arg)
{
    int _sockfd = arg->_sockfd;
    std::set<int>::iterator it;
    std::set<int> *serverList = arg->serverList;
    message byteMsg;
    byteMsg = createTermMsg(TERMINATE);
    for(it = (*serverList).begin(); it != (*serverList).end(); it++)
    {
        if(sendToEntity(*it, byteMsg) == 0)
            (*serverList).erase(it);
    }
    while(!(*serverList).empty())
    {
    }
    *(arg->term) = true;
    return 1;
}

void* handleIncomingConn(void *threadArg)
{
    struct thread_data *arg = (struct thread_data *)threadArg;
    std::set<int> *serverList = arg->serverList;
    int _sockfd = 0;
    memcpy(&_sockfd, &(arg->_sockfd), INT_SIZE);
    void* rcvdMsg;
    message retMsg;
    std::set<int>::iterator it;
    int registered = 0;
    void *reg;
    location *loc;
    while(1)
    {
        if((rcvdMsg = recvFromEntity(_sockfd)) != 0)
        {
            switch(((termMsg*)rcvdMsg)->type)
            {
                case REGISTER:
                    reg = handleRegister((regMsg*)rcvdMsg, arg);
                    if(reg != 0)
                    {
                        loc = (location*)reg;
                        registered = 1;
                    }
                    //else if(registered)
                        //handleDeregister(loc, arg);
                    //printServers();
                    break;
                case LOC_REQUEST:
                    handleLocationRequest((locReqMsg*)rcvdMsg, arg);
                    close(_sockfd);
                    pthread_exit((void *)1);
                case LOC_CACHE_REQUEST:
                    handleCacheLocationRequest((locReqMsg*)rcvdMsg, arg);
                    close(_sockfd);
                    pthread_exit((void *)1);
                case TERMINATE:
                    handleTerminate(arg);
                    close(_sockfd);
                    pthread_exit((void *)-1);
                default:
                    retMsg = createTermMsg(MESSAGE_INVALID);
                    if(sendToEntity(_sockfd, retMsg) < 0)
                    {
                        perror("Reply to request failed");
                        close(_sockfd);
                        pthread_exit((void *)0);
                    }
            }
        }
        else
        {
            //printServers();
            break;
        }
    }
    if((it = (*serverList).find(_sockfd)) != (*serverList).end())
        (*serverList).erase(_sockfd);
    if(registered == 1)
        handleDeregister(loc, arg);
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
                printf("BINDER_ADDRESS %s \n", myName.IP);
                myName.port = getPort(sockfd);
                printf("BINDER_PORT %d \n", myName.port);
                
                return 1;
            }
        }
    }
    return 0;
}



void* startAccept(void *threadArg)
{
    struct thread_data *arg = (struct thread_data *)threadArg;
    int _sockfd = 0;
    memcpy(&_sockfd, &(arg->_sockfd), INT_SIZE);
    int status;
    void *rc;
    pthread_t threads[NUM_THREADS];
    struct thread_data threadDataArr[NUM_THREADS];
    printf("Starting to accept\n");
    int num = 0;
    while(!terminate)
    {
        int newSockfd;
        if(!terminate && (newSockfd = acceptSocket(_sockfd)) > 0)
        {
            threadDataArr[num].serverStore = arg->serverStore;
            threadDataArr[num]._sockfd = newSockfd;
            threadDataArr[num].serverList = arg->serverList;
            threadDataArr[num].term = arg->term;
            status = pthread_create(&threads[num], NULL, handleIncomingConn, (void *)&threadDataArr[num]);
            if(status == -1)
                break;
            num++;
            //printServerStore(*(arg->serverStore));
        }
        else
            break;
    }
    for(int t=0; t<num; t++) 
        pthread_join(threads[t], &rc);
    pthread_exit((void*)0);
}

int run()
{
    int status;
    void *rc;
    pthread_t binderThread;
    struct thread_data binderData;
    terminate = false;
    servList = new std::set<int>;
    binderData.serverStore = &servStore;
    binderData._sockfd = sockfd;
    binderData.serverList = servList;
    binderData.term = &terminate;
    status = pthread_create(&binderThread, NULL, startAccept, (void *)&binderData);
    while(!terminate)
    {
    }
    close(sockfd);
    return 0;
}


int main(void)
{
    listen();
    run();
    return 1;
}


