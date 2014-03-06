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


message* createMsg(char *IP, int port)
{
    message* msg = new message;
    msg->head.type = REGISTER;
    locSucMsg *dat;
    dat->IP = IP;
    dat->port = port;
    msg->dat = (void *)dat;
    size_t len;
    len = strlen(IP) + sizeof(int);
    msg->head.length = len;

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
