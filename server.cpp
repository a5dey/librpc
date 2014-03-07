#include "librpc.h"


int main(void)
{
    rpcInit();
    int argTypes[] =
    {   1,
        0
    };
    rpcRegister("sum", argTypes);
    return 1;
}
