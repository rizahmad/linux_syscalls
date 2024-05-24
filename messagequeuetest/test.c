#include <linux/kernel.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#define LOG(m) printf("%s: %d : %s\n", __FILE__, __LINE__, m)

int main(int argc, char *argv[])
{
    LOG("Hello, World!");

    return 0;
}