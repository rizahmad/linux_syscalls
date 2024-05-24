#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <pthread.h>

#define MAX_BUFFER_SIZE 256
char * kernelBufferPtr = 0u;

typedef struct KernelEvent
{
    int                 event;
    pthread_mutex_lock  lock;
    pthread_cond_t      cond;
};

KernelEvent bufferFilledEvent;
KernelEvent readAckEvent;

void InitEvent(KernelEvent kernelEvent)
{
    kernelEvent.event = 0u;
    kernelEvent.lock = PTHREAD_MUTEX_INITIALIZER;
    kernelEvent.cond = PTHREAD_COND_INITIALIZER;
}

void WaitEvent(KernelEvent kernelEvent)
{
    pthread_mutex_lock(&kernelEvent.lock);
    if(!kernelEvent.event)
    {
        pthread_cond_wait(&kernelEvent.cond, &kernelEvent.lock);
    }
    pthread_mutex_unlock(kernelEvent.lock);
}

void SetEvent(KernelEvent kernelEvent)
{
    pthread_mutex_lock(&kernelEvent.lock);
    kernelEvent.event = 1u;
    pthread_cond_broadcast(&kernelEvent.cond);
    pthread_mutex_unlock(kernelEvent.lock);
}

SYSCALL_DEFINE0(create_queue)
{
    char * ret = 0u;
    printk("create_queue.\n");

    kernelBufferPtr = (char*)malloc(MAX_BUFFER_SIZE * sizeof(char));
    
    if(kernelBufferPtr != 0u)
    {
        ret = kernelBufferPtr;
        InitEvent(bufferFilledEvent);
        InitEvent(readAckEvent);
    }

    return ret;
}

SYSCALL_DEFINE0(delete_queue)
{
    printk("delete_queue.\n");
    
    if(kernelBufferPtr != 0u)
    {
        free(kernelBufferPtr);
        InitEvent(bufferFilledEvent);
        InitEvent(readAckEvent);
    }
    
    return 0;
}

SYSCALL_DEFINE3(msg_send, const char*, messageString, unsigned int, messageLength, unsigned char*, queuePtr)
{
    printk("msg_send.\n");

    kernalSemaphore = 1u;
    
    if (queuePtr != 0u && messageLength <= MAX_BUFFER_SIZE)
    {
        memcpy(queuePtr, messageString, (size_t)messageLength);
    }

    SetEvent(bufferFilledEvent);

    WaitEvent(readAckEvent);

    return 0;
}

SYSCALL_DEFINE3(msg_receive, char *, receiveBufferPtr, unsigned int *, messageLength, unsigned char*, queuePtr)
{
    printk("msg_receive.\n");

    WaitEvent(bufferFilledEvent);
    
    if (queuePtr != 0u && messageLength <= MAX_BUFFER_SIZE)
    {
        memcpy(receiveBufferPtr, queuePtr, (size_t)messageLength);
    }

    return 0;
}

SYSCALL_DEFINE0(msg_ack)
{
    printk("msg_ack.\n");

    SetEvent(readAckEvent);

    InitEvent(bufferFilledEvent);
    InitEvent(readAckEvent);

    return 0;
}