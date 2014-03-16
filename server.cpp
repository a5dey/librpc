#include "librpc.h"

int sum(int *vect, int size)
{
    int i = 0;
    int result = 0;
    while(i < size)
    {
        result += vect[i];
        i++;
    }
    return result;
}


int sumSkeleton(int *argTypes, void **args)
{
    //this skeleton function knows that argTypes[0] is the returned result
    //pointer, argTypes[1] is array of integers to be summed.
    int i = 0;
    int out;
    int in;
    while(argTypes[i] != 0)
    {
        int inout = (argTypes[i] >> ARG_OUTPUT);
        if(inout == 1)
            out = i;
        else if (inout == 2)
            in = i;
        i++;
    }
    int size = argTypes[in] & 0xffff;
    *(int*)args[out] = sum((int*)args[in], size);
    return 1;
}

int main(void)
{
    rpcInit();
    int argTypes[3] =
    {   (1 << ARG_OUTPUT) | (ARG_INT << 16),
        (1 << ARG_INPUT) | (ARG_INT << 16) | 2,
        0
    };
    rpcRegister("sum", argTypes, *sumSkeleton);
    rpcExecute();
    return 1;
}
    
    

