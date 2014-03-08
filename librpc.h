
#define ARG_CHAR 1
#define ARG_SHORT 2
#define ARG_INT 3
#define ARG_LONG 4
#define ARG_DOUBLE 5
#define ARG_FLOAT 6

#define ARG_INPUT 31
#define ARG_OUTPUT 30


typedef int (*skeleton)(int *, void **);

/****** Server and client Functions *********/
int rpcInit(void);
int rpcRegister(char *name, int *argTypes);
int rpcExecute(void);
int rpcCall(char *name, int *argTypes, void **args);

