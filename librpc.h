typedef int (*skeleton)(int *, void **);

/****** Server and client Functions *********/
int rpcInit(void);
int rpcRegister(char *name, int *argTypes, skeleton f);
int rpcExecute(void);
