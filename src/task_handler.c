#include <pthread.h>
#include <signal.h>

#include <sys_mgr.h>
#include <workqueue.h>

extern volatile sig_atomic_t g_run;


void * main_task_handler(void* arg)
{
    work_t *w = NULL;

    printf("Task handler is running...\n");
    while (g_run) {
        usleep(200000);
        printf("Task handler is waiting for new task...\n");
        w = pop_work_wait();
        if (w == NULL) {
            printf("Task handler is exiting...\n");
            break;
        }
        printf("Task: received opcode=%d, data=%s\n", w->opcode, w->data);

        sleep(1);
        printf("Task done: %s\n\n", w->data);

        free(w);
    };

    printf("Task handler thread exiting...\n");
    return NULL;
}
