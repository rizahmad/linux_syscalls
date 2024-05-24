gcc -o client messagequeueclient.c && gcc -o server messagequeueserver.c

gcc -o create_queue create_queue.c && gcc -o delete_queue delete_queue.c && gcc -o send_message send_message.c && gcc -o receive_message receive_message.c && gcc -o ack ack.c