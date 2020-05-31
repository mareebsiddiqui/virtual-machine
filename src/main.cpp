#include <stdio.h>
#include <stdlib.h>

#include "vm.hpp"

int main(int argc, const char* argv[])
{
    /* Load Arguments */
    if (argc < 2)
    {
        /* show usage string */
        printf("lc3 [image-file1] ...\n");
        exit(2);
    }

    /* Start VM */
    VirtualMachine obj(argc, argv);
    obj.fetch_execute();
    obj.shutdown();

}