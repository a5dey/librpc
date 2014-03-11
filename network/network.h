
#include "../message/message.h"

#define PORT "0"
#define BACKLOG 20
#define HOSTNAME_LEN 255


/***** Socket data structures *****/

struct addrInfo{
    char IP[HOSTNAME_LEN];
    int port;
} ;

/***** Socket Functions ****/

void *get_in_addr(struct sockaddr *sa);
int getSocket();
addrinfo* getAddrInfo(char *IP, char *port);
int bindSocket(int sockfd, struct addrinfo *myInfo);
int listenSocket(int sockfd);
int connectSocket(int _sockfd, struct addrinfo *entityInfo);
int acceptSocket(int _sockfd);
int sendToBinder(int _sockfd, message msg);
char* getMyIP();
int getPort(int _sockfd);
void* recvFromEntity(int _sockfd);
void* sendRecvBinder(int _sockfd, message msg);
int sendToEntity(int _sockfd, message msg);
