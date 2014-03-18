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
#include <iostream>
#include <string>
#include "message.h"


message allocMemMsg(size_t len)
{
    message msg = (message)malloc(len);
    return msg;
}

size_t getLengthOfMsg(message msg)
{
    assert(msg != NULL);
    size_t length = 0;
    assert(length == 0);
    memcpy(&length, msg, DATALEN_SIZE);
    assert(length != NULL);
    return length+DATALEN_SIZE;
}

void* convToByte(void *src, void *dest, size_t len, size_t moveBy)
{
    assert(src != NULL);
    memcpy(dest, src, len);
    assert(dest != NULL);
    return (dest + moveBy);
}

void* convFromByte(void *src, void *dest, size_t len)
{
    assert(src != NULL);
    memcpy(dest, src, len);
    assert(dest != NULL);
    return (src + len);
}

size_t getArgTypesLen(int *argTypes)
{
    size_t len = 0;
    int x;
    for(int i = 0; ; i++)
    {
        x = argTypes[i];
        len++;
        if(x == 0)
            break;
    }
    return len*INT_SIZE;
}

size_t getArgTypesLenFromByte(message msg, size_t len)
{
    size_t argTypesLen = 0;
    int x = 1;
    msg = (message)convFromByte(msg, &x, INT_SIZE);
    while(x != 0)
    {
        argTypesLen++;
        msg = (message)convFromByte(msg, &x, INT_SIZE);
    }
    argTypesLen++;
    return argTypesLen*INT_SIZE;
}

skeleArgs* createFuncArgs(char *name, int *argTypes)
{
    printf("Name of function %s\n", name);
    skeleArgs *args = new skeleArgs;
    args->name = (char*)malloc(strlen(name)+1);
    strncpy(args->name, name, strlen(name));
    size_t numArgs = getArgTypesLen(argTypes)/INT_SIZE;
    args->argTypes = new int[numArgs];
    args->argTypes = argTypes;
    return args;
}

location* createLocation(char *IP, int port)
{
    printf("Printing Location from createLocation %s\n", IP);
    location *loc = new location;
    loc->IP = (char*)malloc(strlen(IP)+1);
    strncpy(loc->IP, IP, strlen(IP));
    loc->IP[strlen(IP)] = '\0';
    loc->port = port;
    return loc;
}

termMsg* parseTermMsg(messageType type)
{
    termMsg *prsdMsg = new termMsg;
    prsdMsg->type = type;
    return prsdMsg;
}


exeMsg* parseExeMsg(messageType type, message msg, size_t len)
{
    exeMsg *prsdMsg = new exeMsg;
    prsdMsg->type = type;
    prsdMsg->name = (char*)malloc(FUNCNAME_SIZE);
    msg = (message)convFromByte(msg, prsdMsg->name, FUNCNAME_SIZE);
    size_t argTypesLen = getArgTypesLenFromByte(msg, len-FUNCNAME_SIZE);
    prsdMsg->argTypes = (int*)malloc(argTypesLen);
    msg = (message)convFromByte(msg, prsdMsg->argTypes, argTypesLen);
    size_t argsLen = (argTypesLen-INT_SIZE)*VOID_SIZE/INT_SIZE;
    prsdMsg->args = (void**)malloc(argsLen);
    msg = (message)convFromByte(msg, prsdMsg->args, argsLen);
    return prsdMsg;
}

sucFailMsg* parseSucFailMsg(messageType type, message msg, size_t len)
{
    int err;
    msg = (message)convFromByte(msg, &err, len);
    sucFailMsg *prsdMsg = new sucFailMsg;
    prsdMsg->type = type;
    prsdMsg->reason = err;
    printf("Type %d, err %d\n", prsdMsg->type, prsdMsg->reason);
    return prsdMsg;
}

locSucMsg* parseLocSucMsg(messageType type, message msg, size_t len)
{
    assert(msg != NULL);
    assert(len != NULL);
    locSucMsg *prsdMsg = new locSucMsg;
    prsdMsg->type = type;
    assert(prsdMsg != NULL);
    prsdMsg->IP = (char*)malloc(HOSTNAME_SIZE);
    msg = (message)convFromByte(msg, prsdMsg->IP, HOSTNAME_SIZE);
    msg = (message)convFromByte(msg, &prsdMsg->port, INT_SIZE);
    printf("Type %d, Server identifier %s, port %d\n", prsdMsg->type, prsdMsg->IP, prsdMsg->port);
    assert(prsdMsg != NULL);
    return prsdMsg;
}

regMsg* parseRegMsg(message msg, size_t len)
{
    assert(msg != NULL);
    assert(len != NULL);
    regMsg *prsdMsg = new regMsg;
    prsdMsg->type = REGISTER;
    assert(prsdMsg != NULL);
    prsdMsg->IP = (char*)malloc(HOSTNAME_SIZE);
    msg = (message)convFromByte(msg, prsdMsg->IP, HOSTNAME_SIZE);
    msg = (message)convFromByte(msg, &prsdMsg->port, INT_SIZE);
    prsdMsg->name = (char*)malloc(FUNCNAME_SIZE);
    msg = (message)convFromByte(msg, prsdMsg->name, FUNCNAME_SIZE);
    size_t argTypesLen = len - HOSTNAME_SIZE - INT_SIZE - FUNCNAME_SIZE;
    prsdMsg->argTypes = (int*)malloc(argTypesLen);
    msg = (message)convFromByte(msg, prsdMsg->argTypes, argTypesLen);
    assert(prsdMsg != NULL);
    return prsdMsg;
}

locReqMsg* parseLocMsg(message msg, size_t len)
{
    assert(msg != NULL);
    assert(len != NULL);
    locReqMsg *prsdMsg = new locReqMsg;
    prsdMsg->type = LOC_REQUEST;
    assert(prsdMsg != NULL);
    prsdMsg->name = (char*)malloc(FUNCNAME_SIZE);
    msg = (message)convFromByte(msg, prsdMsg->name, FUNCNAME_SIZE);
    size_t argTypesLen = len - FUNCNAME_SIZE;
    prsdMsg->argTypes = (int*)malloc(argTypesLen);
    msg = (message)convFromByte(msg, prsdMsg->argTypes, argTypesLen);
    assert(prsdMsg != NULL);
    return prsdMsg;
}

void* parseMsg(message msg, size_t msgLen)
{
    assert(msg != NULL);
    assert(msgLen != NULL);
    printf("received msgLen %zu\n", msgLen);
    size_t dataLen = msgLen - TYPE_SIZE;
    messageType type;
    byte *data = msg;
    assert(data != NULL);
    data = (message)convFromByte(data, &type, TYPE_SIZE);
    assert(data != NULL);
    assert(type != NULL);
    switch(type) {
        case REGISTER: return (void*)parseRegMsg(data, dataLen);
        case REGISTER_FAILURE: return (void*)parseSucFailMsg(REGISTER_FAILURE, data, dataLen);
        case REGISTER_SUCCESS: return (void*)parseSucFailMsg(REGISTER_SUCCESS, data, dataLen);
        case EXECUTE_FAILURE: return (void*)parseSucFailMsg(EXECUTE_FAILURE, data, dataLen);
        case EXECUTE: return (void*)parseExeMsg(EXECUTE, data, dataLen);
        case EXECUTE_SUCCESS: return (void*)parseExeMsg(EXECUTE_SUCCESS, data, dataLen);
        case TERMINATE: return (void*)parseTermMsg(TERMINATE);
            //case MESSAGE_INVALID: return (void*)parseTermMsg(MESSAGE_INVALID);
        case LOC_REQUEST: return (void*)parseLocMsg(data, dataLen);
        case LOC_SUCCESS: return (void*)parseLocSucMsg(LOC_SUCCESS, data, dataLen);
        case LOC_FAILURE: return (void*)parseSucFailMsg(LOC_FAILURE, data, dataLen);
            
    }
    return NULL;
}

message createLocReqMsg(messageType type, char *name, int *argTypes)
{
    size_t nameLen = strlen(name);
    size_t argTypesLen = getArgTypesLen(argTypes);
    size_t dataLen = TYPE_SIZE + FUNCNAME_SIZE + argTypesLen;
    message msg = allocMemMsg(dataLen + DATALEN_SIZE);
    byte *data = msg;
    data = (message)convToByte(&dataLen, data, DATALEN_SIZE, DATALEN_SIZE);
    data = (message)convToByte(&type, data, TYPE_SIZE, TYPE_SIZE);
    data = (message)convToByte(name, data, nameLen, FUNCNAME_SIZE);
    data = (message)convToByte(argTypes, data, argTypesLen, argTypesLen);
    return msg;
}

message createRegMsg(char *IP, int port, char *name, int *argTypes)
{
    assert(name != NULL);
    assert(argTypes != NULL);
    assert(IP != NULL);
    assert(port != NULL);
    messageType type = REGISTER;
    size_t IPLen = strlen(IP);
    size_t portLen = INT_SIZE;
    size_t nameLen = strlen(name);
    size_t argTypesLen = getArgTypesLen(argTypes);
    assert(argTypesLen != 0);
    size_t dataLen = TYPE_SIZE + HOSTNAME_SIZE + portLen + FUNCNAME_SIZE + argTypesLen;
    message msg = allocMemMsg(dataLen + DATALEN_SIZE);
    byte *data = msg;
    data = (message)convToByte(&dataLen, data, DATALEN_SIZE, DATALEN_SIZE);
    assert(msg != NULL);
    data = (message)convToByte(&type, data, TYPE_SIZE, TYPE_SIZE);
    assert(data+DATALEN_SIZE != NULL);
    data = (message)convToByte(IP, data, IPLen, HOSTNAME_SIZE);
    assert(data+TYPE_SIZE != NULL);
    data = (message)convToByte(&port, data, portLen, INT_SIZE);
    data = (message)convToByte(name, data, nameLen, FUNCNAME_SIZE);
    data = (message)convToByte(argTypes, data, argTypesLen, argTypesLen);
    assert(msg != NULL);
    return msg;
}

message createLocSucMsg(char *IP, int port)
{
    assert(IP != NULL);
    assert(port != NULL);
    messageType type = LOC_SUCCESS;
    size_t IPLen = strlen(IP);
    size_t portLen = INT_SIZE;
    size_t dataLen = TYPE_SIZE + HOSTNAME_SIZE + portLen;
    message msg = allocMemMsg(dataLen + DATALEN_SIZE);
    byte *data = msg;
    data = (message)convToByte(&dataLen, data, DATALEN_SIZE, DATALEN_SIZE);
    assert(msg != NULL);
    data = (message)convToByte(&type, data, TYPE_SIZE, TYPE_SIZE);
    assert(data+DATALEN_SIZE != NULL);
    data = (message)convToByte(IP, data, IPLen, HOSTNAME_SIZE);
    assert(data+TYPE_SIZE != NULL);
    data = (message)convToByte(&port, data, portLen, INT_SIZE);
    assert(msg != NULL);
    return msg;
}

message createExeSucMsg(messageType type, char *name, int *argTypes, void **args)
{
    size_t dataLen = TYPE_SIZE;
    size_t nameLen = strlen(name);
    size_t argTypesLen = getArgTypesLen(argTypes);
    size_t argsLen = (argTypesLen-INT_SIZE)*VOID_SIZE/INT_SIZE;
    dataLen += FUNCNAME_SIZE + argTypesLen + argsLen;
    message msg = allocMemMsg(dataLen + DATALEN_SIZE);
    byte *data = msg;
    data = (message)convToByte(&dataLen, data, DATALEN_SIZE, DATALEN_SIZE);
    data = (message)convToByte(&type, data, TYPE_SIZE, TYPE_SIZE);
    data = (message)convToByte(name, data, nameLen, FUNCNAME_SIZE);
    data = (message)convToByte(argTypes, data, argTypesLen, argTypesLen);
    data = (message)convToByte(args, data, argsLen, argsLen);
    return msg;
}

message createSucFailMsg(messageType type, int reason)
{
    size_t dataLen = TYPE_SIZE + INT_SIZE;
    message msg = allocMemMsg(dataLen + DATALEN_SIZE);
    byte *data = msg;
    data = (message)convToByte(&dataLen, data, DATALEN_SIZE, DATALEN_SIZE);
    data = (message)convToByte(&type, data, TYPE_SIZE, TYPE_SIZE);
    data = (message)convToByte(&reason, data, INT_SIZE, INT_SIZE);
    return msg;
}

message createTermMsg(messageType type)
{
    size_t dataLen = TYPE_SIZE;
    message msg = allocMemMsg(dataLen + DATALEN_SIZE);
    byte *data = msg;
    data = (message)convToByte(&dataLen, data, DATALEN_SIZE, DATALEN_SIZE);
    data = (message)convToByte(&type, data, TYPE_SIZE, TYPE_SIZE);
    return msg;
}

