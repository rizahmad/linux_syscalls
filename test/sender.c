#include <linux/kernel.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#define __NR_create_queue 463
#define __NR_delete_queue 464
#define __NR_msg_send 465
#define __NR_msg_receive 466
#define __NR_msg_ack 467

#define E_OK 0x0
#define E_NOK 0xFF

#define LOG(m) printf("%s: %d : %s\n", __FILE__, __LINE__, m)

long create_queue_syscall(void)
{
 return syscall(__NR_create_queue);
}

long delete_queue_syscall(void)
{
 return syscall(__NR_delete_queue);
}

long msg_send_syscall(const char* messageString, unsigned int messageLength, unsigned char* queuePtr)
{
 return syscall(__NR_msg_send, messageString, messageLength, queuePtr);
}

long msg_receive_syscall(const char* receiveBufferPtr, unsigned int * messageLength, unsigned char* queuePtr)
{
 return syscall(__NR_msg_receive, receiveBufferPtr, messageLength, queuePtr);
}

long msg_ack_syscall(void)
{
 return syscall(__NR_msg_ack);
}

int main(int argc, char *argv[])
{
    char defaultMessage[13] = "hello, there!";
    int defaultMessageLength = sizeof(defaultMessage)/(sizeof(char));

    char * message;
    int messageLength;
    if (argc > 1u)
    {
        message = argv[1u];
        messageLength = strlen(argv[1u]);
    }
    else
    {
        message = defaultMessage;
        messageLength = defaultMessageLength;
    }
    

    LOG("Getting queue.");
    char* mqPtr = create_queue_syscall();
    if(mqPtr == NULL)
    {
        LOG("create_queue system call returned error.");
        return -1;
    }
    
    LOG("Sending message.");
    if(E_OK != msg_send_syscall(message, messageLength, mqPtr))
    {
        LOG("msg_send system call returned error.");
        return -1;
    }
    
    return 0;
}