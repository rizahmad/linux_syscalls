#include <linux/kernel.h>
#include <linux/syscalls.h>

#define MAX_BUFFER_SIZE 256

char * kernelBufferPtr = NULL;
int kernelMessageLength = 0u;

typedef struct 
{
    int lock;
} MQ_Spinlock;

MQ_Spinlock bufferFilledLock, readAckLock;

void GetSpinlock(MQ_Spinlock spinlock);
void ReleaseSpinlock(MQ_Spinlock spinlock);
void InitSpinlock(MQ_Spinlock spinlock);

void GetSpinlock(MQ_Spinlock spinlock)
{
    while (spinlock.lock == 0u);
    spinlock.lock = 1u;
}

void ReleaseSpinlock(MQ_Spinlock spinlock)
{
    spinlock.lock = 0u;
}

void InitSpinlock(MQ_Spinlock spinlock)
{
    spinlock.lock = 0u;
}

SYSCALL_DEFINE0(create_queue)
{
    char * ret = 0u;
    printk("create_queue.\n");

    kernelBufferPtr = (char*)kmalloc(MAX_BUFFER_SIZE * sizeof(char), GFP_ATOMIC);
    
    if(kernelBufferPtr != NULL)
    {
        ret = kernelBufferPtr;
        InitSpinlock(bufferFilledLock);
        InitSpinlock(readAckLock);
        GetSpinlock(bufferFilledLock);
        GetSpinlock(readAckLock);
    }

    return ret;
}

SYSCALL_DEFINE0(delete_queue)
{
    printk("delete_queue.\n");
    
    if(kernelBufferPtr != NULL)
    {
        kfree(kernelBufferPtr);
        InitSpinlock(bufferFilledLock);
        InitSpinlock(readAckLock);
    }
    
    return 0;
}

SYSCALL_DEFINE3(msg_send, const char*, messageString, unsigned int, messageLength, unsigned char*, queuePtr)
{
    printk("msg_send.\n");
    
    if (queuePtr != NULL && messageLength <= MAX_BUFFER_SIZE)
    {
        memcpy(queuePtr, messageString, (size_t)messageLength);
        kernelMessageLength = messageLength;
    }

    ReleaseSpinlock(bufferFilledLock);

    GetSpinlock(readAckLock);

    return 0;
}

SYSCALL_DEFINE3(msg_receive, char *, receiveBufferPtr, unsigned int *, messageLength, unsigned char*, queuePtr)
{
    printk("msg_receive.\n");

    GetSpinlock(bufferFilledLock);
    
    if (queuePtr != NULL)
    {
        memcpy(receiveBufferPtr, queuePtr, (size_t)messageLength);
        *messageLength = kernelMessageLength;
    }

    return 0;
}

SYSCALL_DEFINE0(msg_ack)
{
    printk("msg_ack.\n");

    ReleaseSpinlock(readAckLock);

    return 0;
}