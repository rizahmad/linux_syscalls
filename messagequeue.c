#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/spinlock.h>

#define MESSAGE_MAX 256
#define QUEUE_MAX 10
#define DEBUG

#ifdef DEBUG
    #define LOG(m) printk("%s: %d : %s\n", __FILE__, __LINE__, m)
#else
    #define LOG(m)
#endif

#define E_OK 0x0
#define E_NOK 0xFF

typedef struct
{
    unsigned int id;
    unsigned int len;
    char * buffer;
    struct mutex ackLock;
    struct mutex receiveLock;
    struct mutex queueLock;
}MessageQueue;

static DEFINE_MUTEX(globalLock);


struct QueueList{
     struct list_head list;
     int queueId;
     MessageQueue * mqPtr;
};

LIST_HEAD(QueueListHeadNode);

void AddMessageQueue(int queueId, MessageQueue * mqPtr);
void RemoveMessageQueue(int queueId);
int FindMessageQueue(int queueId);
int FindMessageQueue(int queueId);
MessageQueue * GetMessageQueue(int queueId);

void AddMessageQueue(int queueId, MessageQueue * mqPtr)
{
    struct QueueList *np = kmalloc(sizeof(struct QueueList), GFP_ATOMIC);
    np->queueId = queueId;
    np->mqPtr = mqPtr;

    INIT_LIST_HEAD(&np->list);

    list_add_tail(&np->list, &QueueListHeadNode);
}

void RemoveMessageQueue(int queueId)
{
    struct QueueList *np, *temp;

    list_for_each_entry_safe(np, temp, &QueueListHeadNode, list) {
            list_del(&np->list);
            kfree(np);
        }
}

int FindMessageQueue(int queueId)
{
    int status = E_NOK;
    struct QueueList *np;

    
    list_for_each_entry(np, &QueueListHeadNode, list) {
        if (np->queueId == queueId)
        {
            status = E_OK;
            break;
        }
    }
    
    return status;
}

MessageQueue * GetMessageQueue(int queueId)
{
    struct QueueList *np;
    MessageQueue * mqPtr = NULL;

    list_for_each_entry(np, &QueueListHeadNode, list) {
        if ( np->queueId == queueId)
        {
            mqPtr = np->mqPtr;
            break;
        }
    }
    
    return mqPtr;
}

SYSCALL_DEFINE1(create_queue, unsigned int, queueId)
{
    LOG("Entering create_queue system call.");

    int status = E_NOK;

    if (E_NOK == FindMessageQueue(queueId))
    {
        LOG("Creating new message queue.");
        MessageQueue * mqPtr = (MessageQueue*)kmalloc(sizeof(MessageQueue), GFP_ATOMIC);
        if (mqPtr != NULL)
        {
            mutex_init(&mqPtr->receiveLock); 
            mutex_init(&mqPtr->ackLock);
            mutex_init(&mqPtr->queueLock);

            LOG("Trying receiveLock.");
            mutex_lock(&mqPtr->receiveLock);
            LOG("Got receiveLock.");
            
            LOG("Trying ackLock.");
            mutex_lock(&mqPtr->ackLock);
            LOG("Got ackLock.");

            AddMessageQueue(queueId, mqPtr);

            status = E_OK;
        }
    }
    else
    {
        LOG("Message queue already exists.");
        status = E_OK;
    }

    LOG("Exiting create_queue system call.");

    return status;
}

SYSCALL_DEFINE1(delete_queue, unsigned int, queueId)
{
    LOG("Entering delete_queue system call.");

    int status = E_NOK;

    if (E_NOK != FindMessageQueue(queueId))
    {
        MessageQueue * mqPtr = GetMessageQueue(queueId);

        /* globalLock is needed here because queueLock will not be available after kfree is called.*/
        mutex_lock(&globalLock);

        LOG("Deleting message buffer.");
        kfree(mqPtr->buffer);
        
        LOG("Unlocking receiveLock.");
        mutex_unlock(&mqPtr->receiveLock);

        LOG("Unlocking ackLock.");
        mutex_unlock(&mqPtr->ackLock);

        LOG("Unlocking queueLock.");
        mutex_unlock(&mqPtr->queueLock);
        
        mutex_unlock(&globalLock);

        LOG("Deleting queue.");
        kfree(mqPtr);

        RemoveMessageQueue(queueId);

        status = E_OK;
    }
    else
    {
        LOG("Queue does not exist.");
    }

    LOG("Exiting delete_queue system call.");
    
    return status;
}

SYSCALL_DEFINE3(msg_send, unsigned int, queueId, char *, message, unsigned int, length)
{
    LOG("Entering msg_send system call.");

    int status = E_NOK;

    if (E_NOK != FindMessageQueue(queueId))
    {
        MessageQueue * mqPtr = GetMessageQueue(queueId);

        mutex_lock(&mqPtr->queueLock);

        LOG("Creating message buffer.");
        mqPtr->buffer = (char*)kmalloc(length * sizeof(char), GFP_ATOMIC);
        if (mqPtr->buffer != NULL)
        {
            LOG("Copying message from user space to kernel space.");
            if (0u != copy_from_user(mqPtr->buffer, message, length))
            {
                LOG("User space to kernel space copy failed.");
                LOG("Releasing receiveLock.");
                mutex_unlock(&mqPtr->receiveLock);
            }
            else
            {
                mqPtr->len = length;

                LOG("Releasing receiveLock.");
                mutex_unlock(&mqPtr->receiveLock);

                LOG("Trying ackLock.");
                mutex_lock(&mqPtr->ackLock);
                LOG("Got ackLock.");

                LOG("Deleting message buffer.");
                kfree(mqPtr->buffer);

                status = E_OK;
            }
        }
        else
        {
            LOG("Could not create message buffer.");
        }

        mutex_unlock(&mqPtr->queueLock);
    }
    else
    {
        LOG("Queue does not exist.");
    }

    LOG("Exiting msg_send system call.");
    
    return status;
}

SYSCALL_DEFINE3(msg_receive, unsigned int, queueId, char *, buffer, unsigned int *, length)
{
    LOG("Entering msg_receive system call.");

    int status = E_NOK;

    if (E_NOK != FindMessageQueue(queueId))
    {
        MessageQueue * mqPtr = GetMessageQueue(queueId);

        mutex_lock(&mqPtr->queueLock);

        LOG("Trying receiveLock.");
        mutex_lock(&mqPtr->receiveLock);
        LOG("Got receiveLock.");

        LOG("Copying message from kernel space to user space.");
        if (0u != copy_to_user(buffer, mqPtr->buffer, mqPtr->len))
        {
            LOG("Copying from kernel space to user space failed.");
        }
        else
        {
            LOG("Copying message length from kernel space to user space.");
            if (0u != copy_to_user(length, &mqPtr->len, sizeof(mqPtr->len)))
            {
                LOG("Copying from kernel space to user space failed.");
            }
            else
            {
                LOG("Copying successful.");
                status = E_OK;
            }
        }

        mutex_unlock(&mqPtr->queueLock);
    }
    else
    {
        LOG("Queue does not exist.");
    }

    LOG("Exiting msg_receive system call.");
    
    return status;
}

SYSCALL_DEFINE1(msg_ack, unsigned int, queueId)
{
    LOG("Entering msg_ack system call.");

    int status = E_NOK;

    if (E_NOK != FindMessageQueue(queueId))
    {
        MessageQueue * mqPtr = GetMessageQueue(queueId);

        LOG("Releasing ackLock.");
    
        mutex_lock(&mqPtr->queueLock);
        mutex_unlock(&mqPtr->ackLock);
        mutex_unlock(&mqPtr->queueLock);

        status = E_OK;
    }
    else
    {
        LOG("Queue does not exist.");
    }

    LOG("Exiting msg_ack system call.");

    return status;
}