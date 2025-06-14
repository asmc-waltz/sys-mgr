#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

#include <workqueue.h>
#include <dbus_comm.h>

extern volatile sig_atomic_t g_run;

static workqueue_t g_wqueue = {
    .head = NULL,
    .tail = NULL,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER
};

work_t * create_work(cmd_data_t *cmd)
{
    work_t *w = NULL;

    w = malloc(sizeof(work_t));
    if (w == NULL || cmd == NULL) {
        return NULL;
    }

    w->cmd = cmd;
    LOG_TRACE("work is created for cmd opcode: %d", w->cmd->opcode);

    return w;
}

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
    work_t *w = NULL;

    pthread_mutex_lock(&g_wqueue.mutex);

    while (!g_wqueue.head && g_run) {
        pthread_cond_wait(&g_wqueue.cond, &g_wqueue.mutex);
    }

    if (g_run == 0) {
        return NULL;
    }

    w = g_wqueue.head;
    g_wqueue.head = w->next;
    if (g_wqueue.head == NULL) {
        g_wqueue.tail = NULL;
    }

    pthread_mutex_unlock(&g_wqueue.mutex);
    return w;
}

void workqueue_stop() {
    pthread_mutex_lock(&g_wqueue.mutex);
    pthread_cond_broadcast(&g_wqueue.cond);
    pthread_mutex_unlock(&g_wqueue.mutex);
}
