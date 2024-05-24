#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/spinlock.h>

#define MAX_BUFFER_SIZE 256
#define DEBUG
#define USE_MUTEX
//#define USE_SPIN_LOCK

#ifdef DEBUG
    #define LOG(m) printk("%s: %d : %s\n", __FILE__, __LINE__, m)
#else
    #define LOG(m)
#endif

#ifdef USE_SPIN_LOCK
    #define DEFINE_LOCK(__lock) DEFINE_SPINLOCK(__lock)
    #define LOCK(__lock) spin_lock(__lock)
    #define UNLOCK(__lock) spin_unlock(__lock)
#elif defined USE_MUTEX
    #define DEFINE_LOCK(__lock) DEFINE_MUTEX(__lock)
    #define LOCK(__lock) mutex_lock(__lock)
    #define UNLOCK(__lock) mutex_unlock(__lock)
#else
    #error Locking premitive not defined (USE_SPIN_LOCK, USE_MUTEX)
#endif


char * kernelBufferPtr = NULL;
int kernelMessageLength = 0u;

DEFINE_LOCK(bufferFilledLock);
DEFINE_LOCK(readAckLock);


SYSCALL_DEFINE0(create_queue)
{
    LOG("Entering create_queue system call.");
    
    char * ret = 0u;

    if(kernelBufferPtr == NULL)
    {
        LOG("Creating kernel buffer.");
        /* New buffer needed */
        kernelBufferPtr = (char*)kmalloc(MAX_BUFFER_SIZE * sizeof(char), GFP_ATOMIC);
        if(kernelBufferPtr != NULL)
        {
            LOG("Trying bufferFilledLock.");
            LOCK(&bufferFilledLock);
            LOG("Got bufferFilledLock.");
            
            LOG("Trying readAckLock.");
            LOCK(&readAckLock);
            LOG("Got readAckLock.");
        }
    }

    ret = kernelBufferPtr;
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
        LOG("Initializing bufferFilledLock.");
        UNLOCK(&bufferFilledLock);
        LOG("Initializing readAckLock.");
        UNLOCK(&readAckLock);
    }
    kernelBufferPtr = NULL;
    LOG("Exiting delete_queue system call.");
    return 0;
}

SYSCALL_DEFINE3(msg_send, const char*, messageString, unsigned int, messageLength, unsigned char*, queuePtr)
{
    LOG("Entering msg_send system call.");
    
    if (queuePtr != NULL && messageLength <= MAX_BUFFER_SIZE)
    {
        LOG("Copying the message to kernel buffer.");
        copy_from_user(queuePtr, messageString, messageLength);
        kernelMessageLength = messageLength;
    }
    LOG("Releasing bufferFilledLock.");
    UNLOCK(&bufferFilledLock);

    LOG("Trying readAckLock.");
    LOCK(&readAckLock);
    LOG("Got readAckLock.");

    LOG("Exiting msg_send system call.");
    return 0;
}

SYSCALL_DEFINE3(msg_receive, char *, receiveBufferPtr, unsigned int *, messageLength, unsigned char*, queuePtr)
{
    LOG("Entering the msg_receive system call.");

    LOG("Trying bufferFilledLock.");
    LOCK(&bufferFilledLock);
    LOG("Got bufferFilledLock.");
    
    if (queuePtr != NULL)
    {
        LOG("Copying message from kernel to user.");
        copy_to_user(receiveBufferPtr, queuePtr, kernelMessageLength);

        LOG("Copying message length from kernel to user.");
        copy_to_user(messageLength, &kernelMessageLength, sizeof(kernelMessageLength));
    }

    LOG("Exiting msg_receive system call.");
    return 0;
}

SYSCALL_DEFINE0(msg_ack)
{
    LOG("Entering msg_ack system call.");

    LOG("Releasing readAckLock.");
    UNLOCK(&readAckLock);

    LOG("Exiting msg_ack system call.");

    return 0;
}