#define CHAR_SIZE (sizeof(char))
#define INT_SIZE (sizeof(int))
#define BYTE_SIZE (sizeof(char))
#define TYPE_SIZE (sizeof(int))
#define VOID_SIZE (sizeof(void*))
#define DATALEN_SIZE (sizeof(int))
#define HEADER_SIZE (TYPE_SIZE+DATALEN_SIZE)
#define MAXDATA_SIZE 10000
#define HEAD_LEN (sizeof(Header))
#define FUNCNAME_SIZE 8
#define HOSTNAME_SIZE 24

typedef unsigned char byte;
typedef byte* message;

enum messageType{
 REGISTER,
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
} ;

enum warning{
    OK,
    FUNC_EXISTS,
    SOCKET_CLOSED,
    LISTEN_ERROR,
    BIND_ERROR,
    INVALID_ARGS,
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
    int reason;
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

/********* FUNCTIONS ************/
message allocMemMsg(size_t len);
size_t getArgTypesLen(int *argTypes);
size_t getArgTypesLenFromByte(message msg, size_t len);
void* convToByte(void *src, void *dest, size_t len, size_t moveBy);
void* convFromByte(void *src, void *dest, size_t len);
size_t getLengthOfMsg(message msg);

/******** PARSING FUNCTIONS **********/
sucFailMsg* parseSucFailMsg(messageType type, message msg, size_t len);
termMsg* parseTermMsg(messageType type);
exeMsg* parseExeMsg(messageType type, message msg, size_t len);
regMsg* parseRegMsg(message msg, size_t len);
void* parseMsg(message msg);

/********* message creating functions **********/
message createRegMsg(char *IP, int port, char *name, int *argTypes);
message createExeSucMsg(messageType type, char *name, int *argTypes, void **args);
message createSucFailMsg(messageType type, int reason);
message createTermMsg(messageType type);
skeleArgs* createFuncArgs(char *name, int *argTypes);
location* createLocation(char *IP, int port);
