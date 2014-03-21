#include <assert.h>
#include <set>
#define CHAR_SIZE (sizeof(char))
#define INT_SIZE (sizeof(int))
#define SHORT_SIZE (sizeof(short))
#define LONG_SIZE (sizeof(long))
#define DOUBLE_SIZE (sizeof(double))
#define FLOAT_SIZE (sizeof(float))
#define BYTE_SIZE (sizeof(char))
#define TYPE_SIZE (sizeof(int))
#define VOID_SIZE (sizeof(void*))
#define DATALEN_SIZE (sizeof(size_t))
#define HEADER_SIZE (TYPE_SIZE+DATALEN_SIZE)
#define MAXDATA_SIZE 10000
#define HEAD_LEN (sizeof(Header))
#define FUNCNAME_SIZE 8 
#define HOSTNAME_SIZE 32

typedef unsigned char byte;
typedef byte* message;

enum messageType{
 REGISTER = 1,
 REGISTER_SUCCESS,
 REGISTER_FAILURE,   
 LOC_REQUEST,   
 LOC_SUCCESS,        
 LOC_FAILURE,        
 EXECUTE,        
 EXECUTE_SUCCESS,     
 EXECUTE_FAILURE,     
 TERMINATE,    
 MESSAGE_INVALID,
 SEND_AGAIN,
 LOC_CACHE_REQUEST,
 LOC_CACHE_SUCCESS,
 LOC_CACHE_FAILURE,
} ;

enum warning{
    CONNECTION_CLOSED = -4,
    BINDER_NOT_FOUND = -3,
    SERVER_NOT_FOUND = -2,
    FUNC_NOT_FOUND = -1,
    OK = 0,
    FUNC_EXISTS = 1,
    SOCKET_CLOSED = 2,
    INVALID_ARGS = 3,
    WARNING = 4,
};

struct Header{
    size_t length;
    messageType type;
};

struct regMsg{
    messageType type;
    char *IP;
    int port;
    char *name;
    int *argTypes;
} ;

struct locReqMsg{
    messageType type;
    char *name;
    int *argTypes;
    void **args;
} ;

struct locSucMsg{
    messageType type;
    char *IP;
    int port;
} ;

struct sucFailMsg{
    messageType type;
    warning reason;
} ;

struct exeMsg{
    messageType type;
    char *name;
    int *argTypes;
    void **args;
} ;

struct termMsg{
    messageType type;
} ;

struct skeleArgs{
    char *name;
    int *argTypes;
};

struct location{
    char *IP;
    int port;
} ;

struct cmp_skeleArgs{
    bool operator()(skeleArgs *a, skeleArgs *b)
    {
        int compName = strcmp(a->name, b->name);
        int compArgTypes = 0;
        int i = 0;
        while(1)
        {
            if(a->argTypes[i] < b->argTypes[i])
            {
                compArgTypes = -1;
                break;
            }
            else if (a->argTypes[i] > b->argTypes[i])
            {
                compArgTypes = 1;
                break;
            }
            if(a->argTypes[i] == 0 || b->argTypes[i] == 0)
                break;
            i++;
        }
        return compName < 0 || (compName == 0 && compArgTypes < 0);
    }
};
//struct cmp_skeleArgs{
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
  
  struct Server {
      location *loc;
      std::set<skeleArgs*, cmp_skeleArgs> *functions;
  };
  
  /********* FUNCTIONS ************/
  message allocMemMsg(size_t len);
  size_t getArgTypesLen(int *argTypes);
  size_t getArgTypesLenFromByte(message msg, size_t len);
size_t getDataTypeLen(int dataType);
  void* convToByte(void *src, void *dest, size_t len, size_t moveBy);
  void* convFromByte(void *src, void *dest, size_t len);
  size_t getLengthOfMsg(message msg);
  
  /******** PARSING FUNCTIONS **********/
  sucFailMsg* parseSucFailMsg(messageType type, message msg, size_t len);
  termMsg* parseTermMsg(messageType type);
  exeMsg* parseExeMsg(messageType type, message msg, size_t len);
  regMsg* parseRegMsg(message msg, size_t len);
  void* parseMsg(message msg, size_t msgLen);
locReqMsg* parseLocMsg(messageType type, message msg, size_t len);
  locSucMsg* parseLocSucMsg(messageType type, message msg, size_t len);

/********* message creating functions **********/
message createRegMsg(char *IP, int port, char *name, int *argTypes);
message createExeSucMsg(messageType type, char *name, int *argTypes, void **args);
message createSucFailMsg(messageType type, warning reason);
message createTermMsg(messageType type);
skeleArgs* createFuncArgs(char *name, int *argTypes);
location* createLocation(char *IP, int port);
message createLocReqMsg(messageType type, char *name, int *argTypes);
message createbndrMsg(messageType type, char *IP, int port);
message createLocSucMsg(char *IP, int port);
message createCacheLocSucMsg(char *IP, int port);
