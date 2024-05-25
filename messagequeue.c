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

#define E_OK 0x0
#define E_NOK 0xFF

static char * kernelBufferPtr = NULL;
static int kernelMessageLength = 0u;

static DEFINE_LOCK(bufferFilledLock);
static DEFINE_LOCK(readAckLock);


SYSCALL_DEFINE0(create_queue)
{
    LOG("Entering create_queue system call.");
    
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

    LOG("Exiting create_queue system call.");

    return kernelBufferPtr;
}

SYSCALL_DEFINE0(delete_queue)
{
    LOG("Entering delete_queue system call.");

    int status = E_NOK;
    
    if(kernelBufferPtr != NULL)
    {
        LOG("Freeing the buffer memory.");
        kfree(kernelBufferPtr);
        kernelBufferPtr = NULL;

        LOG("Initializing bufferFilledLock.");
        UNLOCK(&bufferFilledLock);

        LOG("Initializing readAckLock.");
        UNLOCK(&readAckLock);

        status = E_OK;
    }
    
    LOG("Exiting delete_queue system call.");

    return status;
}

SYSCALL_DEFINE3(msg_send, const char*, messageString, unsigned int, messageLength, unsigned char*, queuePtr)
{
    LOG("Entering msg_send system call.");

    int status = E_NOK;
    
    if (messageString != NULL && messageLength <= MAX_BUFFER_SIZE && queuePtr != NULL) 
    {
        LOG("Copying the message to kernel buffer.");
        if (0u != copy_from_user(queuePtr, messageString, messageLength))
        {
            LOG("User to kernel copy failed.");
            LOG("Releasing bufferFilledLock.");
            UNLOCK(&bufferFilledLock);
        }
        else
        {
            kernelMessageLength = messageLength;
        
            LOG("Releasing bufferFilledLock.");
            UNLOCK(&bufferFilledLock);

            LOG("Trying readAckLock.");
            LOCK(&readAckLock);
            LOG("Got readAckLock.");

            status = E_OK;
        }
    }

    LOG("Exiting msg_send system call.");

    return status;
}

SYSCALL_DEFINE3(msg_receive, char *, receiveBufferPtr, unsigned int *, messageLength, unsigned char*, queuePtr)
{
    LOG("Entering the msg_receive system call.");

    int status = E_NOK;

    LOG("Trying bufferFilledLock.");
    LOCK(&bufferFilledLock);
    LOG("Got bufferFilledLock.");
    
    if (receiveBufferPtr != NULL && messageLength != NULL && queuePtr != NULL)
    {
        LOG("Copying message from kernel to user.");
        if (0u != copy_to_user(receiveBufferPtr, queuePtr, kernelMessageLength))
        {
            LOG("Kernel to user copy failed.");
        }
        else
        {
            LOG("Copying message length from kernel to user.");
            if (0u != copy_to_user(messageLength, &kernelMessageLength, sizeof(kernelMessageLength)))
            {
                LOG("Kernel to user copy failed.");
            }
            else
            {
                LOG("Copying successful.");
                status = E_OK;
            }
        }
    }

    LOG("Exiting msg_receive system call.");

    return status;
}

SYSCALL_DEFINE0(msg_ack)
{
    LOG("Entering msg_ack system call.");

    int status = E_NOK;

    LOG("Releasing readAckLock.");
    UNLOCK(&readAckLock);
    status = E_OK;

    LOG("Exiting msg_ack system call.");

    return status;
}