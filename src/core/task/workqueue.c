/**
 * @file workqueue.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
// #define LOG_LEVEL LOG_LEVEL_TRACE
#if defined(LOG_LEVEL)
#warning "LOG_LEVEL defined locally will override the global setting in this file"
#endif
#include <log.h>

#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>

#include <comm/dbus_comm.h>
#include <sched/workqueue.h>
#include <sched/task.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  GLOBAL VARIABLES
 **********************/
extern volatile sig_atomic_t g_run;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static workqueue_t g_wqueue = {
    .head = NULL,
    .tail = NULL,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER,
    .active_cnt = 0
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
work_t *create_work(uint8_t type, uint8_t priority, uint8_t duration, \
                    uint32_t opcode, void *data)
{
    work_t *w;

    w = calloc(1, sizeof(*w));
    if (!w)
        return NULL;

    w->type = type;
    w->prio = priority;
    w->duration = duration;
    w->opcode = opcode;
    w->data = data;
    LOG_TRACE("Created work for opcode: %d", w->opcode);

    return w;
}

void delete_work(work_t *w)
{
    if (!w) {
        LOG_WARN("Unable to delete work: null work pointer");
        return;
    }

    LOG_TRACE("Deleting work for opcode: %d", w->opcode);

    if (w->data)
        free(w->data);

    free(w);

    /* Decrement active count */
    atomic_fetch_sub(&g_wqueue.active_cnt, 1);

    /* Wake up waiters if queue is drained */
    if (atomic_load(&g_wqueue.active_cnt) == 0) {
        pthread_mutex_lock(&g_wqueue.mutex);
        pthread_cond_broadcast(&g_wqueue.cond);
        pthread_mutex_unlock(&g_wqueue.mutex);
    }
}

void push_work(work_t *w)
{
    pthread_mutex_lock(&g_wqueue.mutex);

    w->next = NULL;
    if (!g_wqueue.tail) {
        g_wqueue.head = g_wqueue.tail = w;
    } else {
        g_wqueue.tail->next = w;
        g_wqueue.tail = w;
    }

    /* Increment active count */
    atomic_fetch_add(&g_wqueue.active_cnt, 1);

    pthread_cond_signal(&g_wqueue.cond);
    pthread_mutex_unlock(&g_wqueue.mutex);
}

work_t *pop_work_wait_safe()
{
    work_t *w = NULL;

    pthread_mutex_lock(&g_wqueue.mutex);

    /* Wait while queue is empty and system is running */
    while (!g_wqueue.head && g_run) {
        pthread_cond_wait(&g_wqueue.cond, &g_wqueue.mutex);
    }

    /*
     * If queue is empty and system is stopping, return NULL
     * Otherwise continue to pop work even if g_run == 0
     */
    if (!g_wqueue.head) {
        pthread_mutex_unlock(&g_wqueue.mutex);
        return NULL;
    }

    /* Pop the first work from queue */
    w = g_wqueue.head;
    g_wqueue.head = w->next;
    if (!g_wqueue.head)
        g_wqueue.tail = NULL;

    pthread_mutex_unlock(&g_wqueue.mutex);
    return w;
}

void workqueue_handler_wakeup() {
    pthread_mutex_lock(&g_wqueue.mutex);
    pthread_cond_broadcast(&g_wqueue.cond);
    pthread_mutex_unlock(&g_wqueue.mutex);
}

int32_t workqueue_active_count(void)
{
    return atomic_load(&g_wqueue.active_cnt);
}
