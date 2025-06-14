/**
 * @file workqueue.h
 *
 */

#ifndef G_WORKQUEUE_H
#define G_WORKQUEUE_H

#include <dbus_comm.h>

typedef struct work {
    int opcode;
    char data[256];
    struct work *next;
} work_t;

typedef struct workqueue {
    work_t *head;
    work_t *tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} workqueue_t;

void push_work(work_t *work);
work_t* pop_work_wait();
void workqueue_stop();

#endif /* G_WORKQUEUE_H */
