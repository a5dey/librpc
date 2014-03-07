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

void* convToByte(void *src, void *dest, size_t len)
{
    memcpy(dest, src, len);
    return dest + len;
}

message createMsg(char *IP, int port)
{
    messageType type = REGISTER;
    size_t dataLen = sizeof(messageType);
    size_t IPLen = strlen(IP);
    size_t portLen = INT_SIZE;
    dataLen += IPLen;
    dataLen += portLen;
    //dataLen += sizeof(dataLen);
    message msg = allocMemMsg(dataLen);
    byte *data = msg;
    data = (message)convToByte(&dataLen, data, 4);
    data = (message)convToByte(&type, data, 4);
    data = (message)convToByte(IP, data, IPLen);
    data = (message)convToByte(&port, data, portLen);
    return msg;
}
    //message* msg = allocMemMsg(&dataLen);
    //msg->head.length = dataLen;
    //msg->head.type = REGISTER;

    //char *buf;
    //buf = IP;
    ////buf = "hello";
    //strcat(buf, std::to_string(port).c_str());
    //printf("Creating msg %s\n", buf);
    //len = strlen(buf);
    //char *msg = new char[4+len];
    //unsigned int value = len + 1;
    //strcpy(msg, std::to_string(value >> 24).c_str());
    //strcpy(msg+1, std::to_string(value >> 16).c_str());
    ////strcpy(msg+2, std::to_string(value >> 8).c_str());
    //*(msg+2) = (char)((value >> 8)+1);
    //*(msg+3) = (char)value;
    ////strcpy(msg+3, std::to_string(value).c_str());
    //strcat(msg+4, buf);


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
//locReqMsg* createLocReqMsg(char *name, int *argTypes)
//{
//    locReqMsg msg;
//    msg.type = LOC_REQUEST;
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
