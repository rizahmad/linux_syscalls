#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/spinlock.h>

#define MAX_BUFFER_SIZE 256

#define LOG(m) printk("%s: %d : %s\n", __FILE__, __LINE__, m)

char * kernelBufferPtr = NULL;
int kernelMessageLength = 0u;

DEFINE_SPINLOCK(bufferFilledLock);
DEFINE_SPINLOCK(readAckLock);


SYSCALL_DEFINE0(create_queue)
{
    LOG("Entering create_queue system call.");
    
    char * ret = 0u;

    LOG("Creating kernel buffer.");
    kernelBufferPtr = (char*)kmalloc(MAX_BUFFER_SIZE * sizeof(char), GFP_ATOMIC);
    
    if(kernelBufferPtr != NULL)
    {
        ret = kernelBufferPtr;
        LOG("Getting spinlocks");
        spin_lock(&bufferFilledLock);
        spin_lock(&readAckLock);
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
        spin_unlock(&bufferFilledLock);
        spin_unlock(&readAckLock);
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
    spin_unlock(&bufferFilledLock);

    LOG("Getting the readAckLock spinlock.");
    spin_lock(&readAckLock);

    LOG("Exiting msg_send system call.");
    return 0;
}

SYSCALL_DEFINE3(msg_receive, char *, receiveBufferPtr, unsigned int *, messageLength, unsigned char*, queuePtr)
{
    LOG("Entering the msg_receive system call.");

    LOG("Getting the bufferFilledLock spinlock.");
    spin_lock(&bufferFilledLock);
    
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
    spin_unlock(&readAckLock);

    LOG("Exiting msg_ack system call.");

    return 0;
}