#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>

#include <workqueue.h>
#include <dbus_comm.h>

extern volatile sig_atomic_t g_run;

static workqueue_t g_wqueue = {
    .head = NULL,
    .tail = NULL,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER
};

work_t *create_work(uint32_t type, void *data)
{
	work_t *work;

	if (!data) {
		LOG_WARN("Unable to create work: invalid data pointer");
		return NULL;
    }

	work = calloc(1, sizeof(*work));
	if (!work)
		return NULL;

    work->type = type;
	work->data = data;
    if (work->type == REMOTE_WORK) {
	    LOG_TRACE("Created work for opcode: %d", ((cmd_data_t *)data)->opcode);
    }

	return work;
}

void delete_work(work_t *work)
{
	void *data;

	if (!work) {
		LOG_WARN("Unable to delete work: null work pointer");
		return;
	}

	data = work->data;
	if (!data) {
		LOG_WARN("Unable to delete work: null data pointer");
		free(work);
		return;
	}

    if (work->type == REMOTE_WORK) {
	    LOG_TRACE("Deleting work for opcode: %d", ((cmd_data_t *)data)->opcode);
    }

	free(data);
	free(work);
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

