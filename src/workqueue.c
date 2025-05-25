#include <pthread.h>
#include <workqueue.h>

static workqueue_t g_wqueue = {
    .head = NULL,
    .tail = NULL,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER
};

void push_work(work_t *w) {
    pthread_mutex_lock(&g_wqueue.mutex);

    w->next = NULL;
    if (!g_wqueue.tail) {
        g_wqueue.head = g_wqueue.tail = w;
    } else {
        g_wqueue.tail->next = w;
        g_wqueue.tail = w;
    }
    pthread_cond_signal(&g_wqueue.cond);

    pthread_mutex_unlock(&g_wqueue.mutex);
}

work_t * pop_work_wait() {
    pthread_mutex_lock(&g_wqueue.mutex);

    while (!g_wqueue.head) {
        pthread_cond_wait(&g_wqueue.cond, &g_wqueue.mutex);
    }

    work_t *w = g_wqueue.head;
    g_wqueue.head = w->next;
    if (!g_wqueue.head) {
        g_wqueue.tail = NULL;
    }

    pthread_mutex_unlock(&g_wqueue.mutex);
    return w;
}

