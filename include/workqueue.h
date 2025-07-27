/**
 * @file workqueue.h
 *
 */

#ifndef G_WORKQUEUE_H
#define G_WORKQUEUE_H

typedef enum {
    LOCAL_WORK = 0,
    REMOTE_WORK,
} work_types;

typedef struct work {
    unsigned int type;
    void *data;
    struct work *next;
} work_t;

typedef struct workqueue {
    work_t *head;
    work_t *tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} workqueue_t;

work_t *create_work(uint32_t type, void *data);
void delete_work(work_t *work);
void push_work(work_t *work);
work_t* pop_work_wait();
void workqueue_stop();

#endif /* G_WORKQUEUE_H */
