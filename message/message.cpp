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
    message msg = (message)calloc(len + 4, BYTE_SIZE);
   return msg;
}

sucFailMsg* parseRegSucMsg(message msg, size_t len)
{
    int err;
    convToByte(msg, &err, len);
    sucFailMsg *prsdMsg = new sucFailMsg;
    prsdMsg->type = REGISTER_SUCCESS;
    prsdMsg->reason = err;
    printf("Type %d, err %d\n", prsdMsg->type, prsdMsg->reason);
    return prsdMsg;
}

regMsg* parseRegMsg(message msg, size_t len)
{
    regMsg *prsdMsg = new regMsg;
    prsdMsg->type = REGISTER;
    prsdMsg->IP = (char*)malloc(24);
    convToByte(msg, prsdMsg->IP, 24);
    convToByte(msg, &prsdMsg->port, INT_SIZE);
    prsdMsg->name = (char*)malloc(10);
    convToByte(msg, prsdMsg->name, 10);
    size_t argTypesLen = len - 24 - INT_SIZE - 10;
    prsdMsg->argTypes = (int*)malloc(argTypesLen);
    convToByte(msg, prsdMsg->argTypes, argTypesLen);
    printf("Server identifier %s, port %d, name of function %s\n", prsdMsg->IP, prsdMsg->port, prsdMsg->name);
    return prsdMsg;
}

//Ankita:please check for compilation errors.
//locReqMsg* parseLocMsg(message msg, size_t len)
//{
//    locReqMsg *prsdMsg = new locReqMsg;
//    prsdMsg->type = LOC_REQUEST;
//    convToByte(msg, prsdMsg->name, 10);
//    convToByte(msg, prsdMsg->argTypes, argTypesLen);
//    convToByte(msg, prsdMsg->args, 10);
//    return prsdMsg;
//}

void* parseMsg(message msg)
{
    size_t dataLen;
    dataLen = getLengthOfMsg(msg);
    printf("received dataLen %zu\n", dataLen);
    messageType type;
    convToByte(msg, &type, TYPE_SIZE);
    switch(type) {
        case REGISTER: return (void*)parseRegMsg(msg+HEADER_SIZE, dataLen - TYPE_SIZE);
        case REGISTER_SUCCESS: return (void*)parseRegSucMsg(msg+HEADER_SIZE, dataLen - TYPE_SIZE);
                       break;
    }
    return NULL;
}

void* convToByte(void *src, void *dest, size_t len)
{
    memcpy(dest, src, len);
    return dest + len;
}

size_t getLengthOfMsg(message msg)
{
    size_t length;
    convToByte(msg, &length, DATALEN_SIZE);
    return length+DATALEN_SIZE;
}

size_t getArgTypesLen(int *argTypes)
{
    size_t len = 0;
    int x;
    for(int i = 0; ; i++)
    {
        x = argTypes[i];
        if(x == 0)
            break;
        else
            len++;
    }
    return len;
}

skeleArgs* createFuncArgs(char *name, int *argTypes)
{
    skeleArgs *args;
    args->name = (char*)malloc(strlen(name));
    strcpy(args->name, name);
    args->argTypes = new int[getArgTypesLen(argTypes)];
    args->argTypes = argTypes;
    return args;
}


//Ankita:please check for compilation errors.
//size_t getArgsLen(void **args)
//{
//    size_t len = 0;
//    int x;
//    for(int i = 0; ; i++)
//    {
//        x = args[i];
//        if(x == 0)
//            break;
//        else
//            len++;
//    }
//    return len;
//}

//Ankita:please check for compilation errors.
//message createLocReqMsg(char *name, int *argTypes, void **args)
//{
//    messageType type = LOC_REQUEST;
//    size_t nameLen = strlen(name);
//    size_t argTypesLen = getArgTypesLen(argTypes);
//    size_t argsLen = getArgsLen(args);
//    dataLen += nameLen + argTypesLen + argsLen;
//    message msg = allocMemMsg(dataLen + DATALEN_SIZE);
//    byte *data = msg;
//    data = (message)convToByte(&dataLen, data, DATALEN_SIZE);
//    data = (message)convToByte(name, data, nameLen);
//    data = (message)convToByte(argTypes, data, argTypesLen);
//    data = (message)convToByte(args, data, argsLen);
//    parseLocMsg(msg_HEADER_SIZE, dataLen - TYPE_SIZE);
//    return msg;
//}

message createRegMsg(char *IP, int port, char *name, int *argTypes)
{
    messageType type = REGISTER;
    size_t dataLen = TYPE_SIZE;
    size_t IPLen = strlen(IP);
    size_t portLen = INT_SIZE;
    size_t nameLen = strlen(name);
    size_t argTypesLen = getArgTypesLen(argTypes)*INT_SIZE;
    dataLen += IPLen + portLen + nameLen + argTypesLen;
    message msg = allocMemMsg(dataLen + DATALEN_SIZE);
    byte *data = msg;
    data = (message)convToByte(&dataLen, data, DATALEN_SIZE);
    data = (message)convToByte(&type, data, TYPE_SIZE);
    data = (message)convToByte(IP, data, IPLen);
    data = (message)convToByte(&port, data, portLen);
    data = (message)convToByte(name, data, nameLen);
    data = (message)convToByte(argTypes, data, argTypesLen);
    //parseRegMsg(msg+HEADER_SIZE, dataLen - TYPE_SIZE);
    return msg;
}

message createExeSucMsg(messageType type, char *name, int *argTypes, void **args)
{
    size_t dataLen = TYPE_SIZE;
    size_t nameLen = strlen(name);
    size_t argTypesLen = getArgTypesLen(argTypes)*INT_SIZE;
    size_t argsLen = argTypesLen*VOID_SIZE;
    dataLen += nameLen + argTypesLen + argsLen;
    message msg = allocMemMsg(dataLen + DATALEN_SIZE);
    byte *data = msg;
    data = (message)convToByte(&dataLen, data, DATALEN_SIZE);
    data = (message)convToByte(&type, data, TYPE_SIZE);
    data = (message)convToByte(name, data, nameLen);
    data = (message)convToByte(argTypes, data, argTypesLen);
    data = (message)convToByte(args, data, argsLen);
    return msg;
}

message createSucFailMsg(messageType type, int reason)
{
    size_t dataLen = TYPE_SIZE + INT_SIZE;
    message msg = allocMemMsg(dataLen + DATALEN_SIZE);
    byte *data = msg;
    data = (message)convToByte(&dataLen, data, DATALEN_SIZE);
    data = (message)convToByte(&type, data, TYPE_SIZE);
    data = (message)convToByte(&reason, data, INT_SIZE);
    return msg;
}

message createTermMsg(messageType type)
{
    size_t dataLen = TYPE_SIZE;
    message msg = allocMemMsg(dataLen + DATALEN_SIZE);
    byte *data = msg;
    data = (message)convToByte(&dataLen, data, DATALEN_SIZE);
    data = (message)convToByte(&type, data, TYPE_SIZE);
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
