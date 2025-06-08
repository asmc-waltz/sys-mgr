#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <log.h>
#include <sys_mgr.h>
#include <workqueue.h>

extern volatile sig_atomic_t g_run;

void * main_task_handler(void* arg)
{
    work_t *w = NULL;

    LOG_INFO("Task handler is running...");
    while (g_run) {
        usleep(200000);
        LOG_DEBUG("Task handler is waiting for new task...");
        w = pop_work_wait();
        if (w == NULL) {
            LOG_INFO("Task handler is exiting...");
            break;
        }
        LOG_TRACE("Task: received opcode=%d, data=%s", w->opcode, w->data);

/******************************************************************************/
        LOG_INFO("#############################################");
        // wifi_connected_ap_get("wlu1u3i2");
        wifi_scan_and_get_results("wlu1u3i2", 1);
        // g_nm_device_get_by_iface("wlu1u3i2");
        // disconnect_interface("wlu1u3i2");
        wifi_connect_to_ssid("wlu1u3i2", "Danh P3", "10110100101");
        LOG_INFO("#############################################");
/******************************************************************************/

        LOG_TRACE("Task done: %s", w->data);
        free(w);
    };

    LOG_INFO("Task handler thread exiting...");
    return NULL;
}
