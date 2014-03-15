//
//  client.cpp
//  bn
//
//  Created by Ankita Dey on 14/03/14.
//  Copyright (c) 2014 Ankita Dey. All rights reserved.
//

#include <stdlib.h>
#include "librpc.h"
#define LENGTH 23
#define PARAMETER_COUNT 2

int main(void)
{
    void **args = (void **)malloc(PARAMETER_COUNT * sizeof(void *));
    int argTypes[PARAMETER_COUNT+1] =
    {   (1 << ARG_OUTPUT) | (ARG_INT << 16),
        (1 << ARG_INPUT) | (ARG_INT << 16) | LENGTH
    };
    rpcCall("sum", argTypes, args);
    rpcCacheCall("sum", argTypes, args);
    return 1;
}

