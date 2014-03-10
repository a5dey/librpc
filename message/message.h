#define CHAR_SIZE (sizeof(char))
#define INT_SIZE (sizeof(int))
#define BYTE_SIZE (sizeof(char))
#define TYPE_SIZE (sizeof(int))
#define VOID_SIZE (sizeof(void*))
#define DATALEN_SIZE (sizeof(int))
#define HEADER_SIZE (TYPE_SIZE+DATALEN_SIZE)
#define MAXDATA_SIZE 10000
#define HEAD_LEN (sizeof(Header))

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
} ;

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


/********* FUNCTIONS ************/
message allocMemMsg(size_t len);
size_t getArgTypesLen(int *argTypes);
void* convToByte(void *src, void *dest, size_t len);
size_t getLengthOfMsg(message msg);

/******** PARSING FUNCTIONS **********/
regMsg* parseRegMsg(message msg, size_t len);
sucFailMsg* parseRegSucMsg(message msg, size_t len);
void* parseMsg(message msg);

/********* message creating functions **********/
message createRegMsg(char *IP, int port, char *name, int *argTypes);
skeleArgs* createFuncArgs(char *name, int *argTypes);
message createExeSucMsg(messageType type, char *name, int *argTypes, void **args);
message createSucFailMsg(messageType type, int reason);
message createTermMsg(messageType type);
