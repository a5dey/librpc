#include "librpc.h"

int sumSkeleton(int *argTypes, void **args)
{
    return 1;
}

int main(void)
{
    rpcInit();
    int argTypes[] =
    {   1,
        0
    };
    rpcRegister("sum", argTypes, *sumSkeleton);
    return 1;
}

