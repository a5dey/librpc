/*
#define REGISTER            1
#define REGISTER_SUCCESS    2
#define REGISTER_FAILURE    3
#define LOC_REQUEST         4
#define LOC_SUCCESS         5
#define LOC_FAILURE         6
#define EXECUTE             7
#define EXECUTE_SUCCESS     8
#define EXECUTE_FAILURE     9
#define TERMINATE           10
*/


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
} ;

struct Header{
    size_t length;
    messageType type;
};

struct regMsg{
    char *IP;
    int port;
    char *name;
    int *argTypes;
} ;

struct locReqMsg{
    char *name;
    int *argTypes;
} ;

struct locSucMsg{
    char *IP;
    int port;
} ;

struct sucFailMsg{
    int reason;
} ;

struct exeMsg{
    char *name;
    int *argTypes;
    void **args;
} ;

//struct termMsg{
//} ;

typedef void* data;

struct message{
    Header head; 
    data dat;
};

message* createMsg(char *IP, int port);