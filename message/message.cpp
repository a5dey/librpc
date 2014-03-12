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
    size_t length;
    memcpy(&length,msg, DATALEN_SIZE);
    return length+DATALEN_SIZE;
}

void* convToByte(void *src, void *dest, size_t len, size_t moveBy)
{
    memcpy(dest, src, len);
    return (dest + moveBy);
}

void* convFromByte(void *src, void *dest, size_t len)
{
    memcpy(dest, src, len);
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
    return argTypesLen*INT_SIZE;
}

skeleArgs* createFuncArgs(char *name, int *argTypes)
{
    printf("Name of function %s\n", name);
    skeleArgs *args = new skeleArgs;
    args->name = (char*)malloc(strlen(name));
    strcpy(args->name, name);
    size_t numArgs = getArgTypesLen(argTypes)/INT_SIZE;
    args->argTypes = new int[numArgs];
    args->argTypes = argTypes;
    return args;
}

location* createLocation(char *IP, int port)
{
    location *loc = new location;
    loc->IP = (char*)malloc(strlen(IP));
    strcpy(loc->IP, IP);
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
    size_t argTypesLen = getArgTypesLenFromByte(msg+FUNCNAME_SIZE, len-FUNCNAME_SIZE);
    prsdMsg->argTypes = (int*)malloc(argTypesLen);
    msg = (message)convFromByte(msg, prsdMsg->argTypes, argTypesLen);
    size_t argsLen = argTypesLen*VOID_SIZE/INT_SIZE;
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

locSucMsg* parseLocSucMsg(message msg, size_t len)
{
    locSucMsg *prsdMsg = new locSucMsg;
    prsdMsg->type = LOC_SUCCESS;
    prsdMsg->IP = (char*)malloc(HOSTNAME_SIZE);
    msg = (message)convFromByte(msg, prsdMsg->IP, HOSTNAME_SIZE);
    msg = (message)convFromByte(msg, &prsdMsg->port, INT_SIZE);
    printf("Type %d, Server identifier %s, port %d\n", prsdMsg->type, prsdMsg->IP, prsdMsg->port);
    return prsdMsg;
}

regMsg* parseRegMsg(message msg, size_t len)
{
    regMsg *prsdMsg = new regMsg;
    prsdMsg->type = REGISTER;
    prsdMsg->IP = (char*)malloc(HOSTNAME_SIZE);
    msg = (message)convFromByte(msg, prsdMsg->IP, HOSTNAME_SIZE);
    msg = (message)convFromByte(msg, &prsdMsg->port, INT_SIZE);
    prsdMsg->name = (char*)malloc(FUNCNAME_SIZE);
    msg = (message)convFromByte(msg, prsdMsg->name, FUNCNAME_SIZE);
    size_t argTypesLen = len - HOSTNAME_SIZE - INT_SIZE - FUNCNAME_SIZE;
    prsdMsg->argTypes = (int*)malloc(argTypesLen);
    msg = (message)convFromByte(msg, prsdMsg->argTypes, argTypesLen);
    return prsdMsg;
}

locReqMsg* parseLocMsg(message msg, size_t len)
{
    locReqMsg *prsdMsg = new locReqMsg;
    prsdMsg->type = LOC_REQUEST;
    prsdMsg->name = (char*)malloc(FUNCNAME_SIZE);
    msg = (message)convFromByte(msg, prsdMsg->name, FUNCNAME_SIZE);
    size_t argTypesLen = len - HOSTNAME_SIZE - INT_SIZE - FUNCNAME_SIZE;;
    prsdMsg->argTypes = (int*)malloc(argTypesLen);
    msg = (message)convFromByte(msg, prsdMsg->argTypes, argTypesLen);
    return prsdMsg;
}

void* parseMsg(message msg)
{
    size_t msgLen;
    msgLen = getLengthOfMsg(msg);
    printf("received msgLen %zu\n", msgLen);
    size_t dataLen = msgLen - HEADER_SIZE;
    messageType type;
    convFromByte(msg+DATALEN_SIZE, &type, TYPE_SIZE);
    switch(type) {
        case REGISTER: return (void*)parseRegMsg(msg+HEADER_SIZE, dataLen);
        case REGISTER_FAILURE: return (void*)parseSucFailMsg(REGISTER_FAILURE, msg+HEADER_SIZE, dataLen);
        case REGISTER_SUCCESS: return (void*)parseSucFailMsg(REGISTER_SUCCESS, msg+HEADER_SIZE, dataLen);
        case EXECUTE_FAILURE: return (void*)parseSucFailMsg(EXECUTE_FAILURE, msg+HEADER_SIZE, dataLen);
        case EXECUTE: return (void*)parseExeMsg(EXECUTE, msg+HEADER_SIZE, dataLen);
        case EXECUTE_SUCCESS: return (void*)parseExeMsg(EXECUTE_SUCCESS, msg+HEADER_SIZE, dataLen);
        case TERMINATE: return (void*)parseTermMsg(TERMINATE);
        //case MESSAGE_INVALID: return (void*)parseTermMsg(MESSAGE_INVALID);
        case LOC_REQUEST: return (void*)parseLocMsg(msg+HEADER_SIZE, dataLen);
        case LOC_SUCCESS: return (void*)parseLocSucMsg(msg+HEADER_SIZE, dataLen);
        case LOC_FAILURE: return (void*)parseSucFailMsg(LOC_FAILURE, msg+HEADER_SIZE, dataLen);

    }
    return NULL;
}

message createLocReqMsg(messageType type, char *name, int *argTypes)
{
    size_t dataLen = TYPE_SIZE;
    size_t nameLen = strlen(name);
    size_t argTypesLen = getArgTypesLen(argTypes);
    dataLen += nameLen + argTypesLen;
    message msg = allocMemMsg(dataLen + DATALEN_SIZE);
    byte *data = msg;
    data = (message)convToByte(&type, data, TYPE_SIZE);
    data = (message)convToByte(&dataLen, data, DATALEN_SIZE);
    data = (message)convToByte(name, data, nameLen);
    data = (message)convToByte(argTypes, data, argTypesLen);
    parseLocMsg(msg+HEADER_SIZE, dataLen - TYPE_SIZE);
    return msg;
}

message createRegMsg(char *IP, int port, char *name, int *argTypes)
{
    messageType type = REGISTER;
    size_t IPLen = strlen(IP);
    size_t portLen = INT_SIZE;
    size_t nameLen = strlen(name);
    size_t argTypesLen = getArgTypesLen(argTypes);
    size_t dataLen = TYPE_SIZE + HOSTNAME_SIZE + portLen + FUNCNAME_SIZE + argTypesLen;
    message msg = allocMemMsg(dataLen + DATALEN_SIZE);
    byte *data = msg;
    data = (message)convToByte(&dataLen, data, DATALEN_SIZE, DATALEN_SIZE);
    data = (message)convToByte(&type, data, TYPE_SIZE, TYPE_SIZE);
    data = (message)convToByte(IP, data, IPLen, HOSTNAME_SIZE);
    data = (message)convToByte(&port, data, portLen, INT_SIZE);
    data = (message)convToByte(name, data, nameLen, FUNCNAME_SIZE);
    data = (message)convToByte(argTypes, data, argTypesLen, argTypesLen);
    //parseRegMsg(msg+HEADER_SIZE, dataLen - TYPE_SIZE);
    return msg;
}

message createbndrMsg(messageType type, char *IP, int port)
{
    size_t dataLen = TYPE_SIZE;
    size_t IPLen = strlen(IP);
    size_t portLen = INT_SIZE;
    dataLen += IPLen + portLen;
    message msg = allocMemMsg(dataLen + DATALEN_SIZE);
    byte *data = msg;
    data = (message)convToByte(&dataLen, data, DATALEN_SIZE);
    data = (message)convToByte(&type, data, TYPE_SIZE);
    data = (message)convToByte(IP, data, IPLen);
    data = (message)convToByte(&port, data, portLen);
    return msg;
}

message createExeSucMsg(messageType type, char *name, int *argTypes, void **args)
{
    size_t dataLen = TYPE_SIZE;
    size_t nameLen = strlen(name);
    size_t argTypesLen = getArgTypesLen(argTypes);
    size_t argsLen = argTypesLen*VOID_SIZE;
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

//regMsg* createRegMsg(addrInfo *identifier, char *name, int *argTypes)
//{
//    regMsg msg;
//    msg.type = REGISTER;
//    msg.IP = identifier->IP;
//    msg.port = identifier->port;
//    msg.name = name;
//    msg.argTypes = argTypes;
//    return &msg;
//}
//

//locSucMsg* createlocSucMsg(addrInfo *identifier)
//{
//    locSucMsg msg;
//    msg.type = LOC_SUCCESS;
//    msg.IP = identifier->IP;
//    msg.port = identifier->port;
//    return &msg;
//}
