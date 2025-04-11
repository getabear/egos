/* Implementing a queue data structure helps you implement cooperative threads,
 * but this is optional and you can maintain arrays instead of queues.
 */

#include "queue.h"
#include <stdlib.h>
#include <stdio.h>





queue_t queue_new() {
    // printf("queue_new\n\r");s
    queue_t queue = malloc(sizeof(struct queue));
    /* Student's code goes here (Cooperative Threads). */
    for(int i = 0; i < _BUF_SIZE; i++){
        queue->buf[i] = NULL;
    }
    queue->left = 0;
    queue->right = 0;
    /* Student's code ends here. */
    // printf("queue_new end\n\r");
    return queue;
}

int queue_enqueue(queue_t queue, void* item) {
    /* Student's code goes here (Cooperative Threads). */
    int idx = (queue->right + 1) % _BUF_SIZE;
    // 队列满了
    if(idx == queue->left){
        return -1;
    }
    queue->buf[queue->right] = item;
    queue->right = idx;
    /* Student's code ends here. */
    return 0;
}

int queue_dequeue(queue_t queue, void** pitem) {
    /* Student's code goes here (Cooperative Threads). */
    // 队列为空
    if(queue->left == queue->right){
        return -1;
    }
    *pitem = queue->buf[queue->left];
    queue->left = (queue->left + 1) % _BUF_SIZE;
    /* Student's code ends here. */
    return 0;
}
