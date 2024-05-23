#include <linux/kernel.h>
#include <linux/syscalls.h>

#define MAX_BUFFER_SIZE 256

#define LOG(m) printk("%s: %d : %s\n", __FILE__, __LINE__, m)

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
    LOG("Entering GetSpinlock.");
    if(spinlock.lock==1)
    {
        LOG("Spinlock is not free.");
    }
    while (spinlock.lock == 1u);
    spinlock.lock = 1u;

    
    LOG("Exiting GetSpinlock.");
}

void ReleaseSpinlock(MQ_Spinlock spinlock)
{
    LOG("Entering ReleaseSpinlock.");

    spinlock.lock = 0u;
    
    LOG("Exiting ReleaseSpinlock.");
}

void InitSpinlock(MQ_Spinlock spinlock)
{
    LOG("Entering InitSpinlock.");

    spinlock.lock = 0u;

    LOG("Exitting InitSpinlock.");
}

SYSCALL_DEFINE0(create_queue)
{
    LOG("Entering create_queue system call.");
    
    char * ret = 0u;

    LOG("Creating kernel buffer.");
    kernelBufferPtr = (char*)kmalloc(MAX_BUFFER_SIZE * sizeof(char), GFP_ATOMIC);
    
    if(kernelBufferPtr != NULL)
    {
        ret = kernelBufferPtr;
        LOG("Initializing spinlocks");
        InitSpinlock(bufferFilledLock);
        InitSpinlock(readAckLock);
        LOG("Getting spinlocks");
        GetSpinlock(bufferFilledLock);
        GetSpinlock(readAckLock);
    }


    LOG("Exiting create_queue system call.");
    return ret;
}

SYSCALL_DEFINE0(delete_queue)
{
    LOG("Entering delete_queue system call.");
    
    if(kernelBufferPtr != NULL)
    {
        LOG("Freeing the buffer memory.");
        kfree(kernelBufferPtr);
        LOG("Initializing spinlocks.");
        InitSpinlock(bufferFilledLock);
        InitSpinlock(readAckLock);
    }
    
    LOG("Exting delete_queue system call.");
    return 0;
}

SYSCALL_DEFINE3(msg_send, const char*, messageString, unsigned int, messageLength, unsigned char*, queuePtr)
{
    LOG("Entering msg_send system call.");
    
    if (queuePtr != NULL && messageLength <= MAX_BUFFER_SIZE)
    {
        LOG("Copying the message to kernel buffer.");
        memcpy(queuePtr, messageString, (size_t)messageLength);
        kernelMessageLength = messageLength;
    }
    LOG("Releasing bufferFilledLock spinlock.");
    ReleaseSpinlock(bufferFilledLock);

    LOG("Getting the readAckLock spinlock.");
    GetSpinlock(readAckLock);

    LOG("Exiting msg_send system call.");
    return 0;
}

SYSCALL_DEFINE3(msg_receive, char *, receiveBufferPtr, unsigned int *, messageLength, unsigned char*, queuePtr)
{
    LOG("Entering the msg_receive system call.");

    LOG("Getting the bufferFilledLock spinlock.");
    GetSpinlock(bufferFilledLock);
    
    if (queuePtr != NULL)
    {
        LOG("Copying the kernel buffer to user buffer.");
        memcpy(receiveBufferPtr, queuePtr, (size_t)kernelMessageLength);
        *messageLength = kernelMessageLength;
    }

    LOG("Exiting msg_receive system call.");
    return 0;
}

SYSCALL_DEFINE0(msg_ack)
{
    LOG("Entering msg_ack system call.");

    LOG("Releasing readAckLock spinlock.");
    ReleaseSpinlock(readAckLock);

    LOG("Exiting msg_ack system call.");

    return 0;
}