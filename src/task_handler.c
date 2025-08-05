#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <log.h>
#include <sys_mgr.h>
#include <sys_comm.h>
#include <workqueue.h>
#include <dbus_comm.h>

extern volatile sig_atomic_t g_run;

void * main_task_handler(void* arg)
{
    work_t *w = NULL;

    LOG_INFO("Task handler is running...");
    while (g_run) {
        usleep(200000);
        LOG_DEBUG("[Task handler] --> waiting for new task...");
        w = pop_work_wait();
        if (w == NULL) {
            LOG_INFO("Task handler is exiting...");
            break;
        }

        if (w->type == REMOTE_WORK) {
            LOG_TRACE("Task: received opcode=%d", ((cmd_data_t *)w->data)->opcode);
        }

/******************************************************************************/
        LOG_INFO("#############################################");
        // wifi_is_connected_to_ssid("wlu1u3i2", "Danh P3");
        // wifi_list_access_points("wlu1u3i2");
        // g_nm_device_get_by_iface("wlu1u3i2");
        // disconnect_interface("wlu1u3i2");
        // wifi_connect_to_ssid("wlu1u3i2", "Danh P3", "10110100101");
        wifi_connect_to_ssid("wlu1u3i2", "Phuong Nguyen iP", "phuongnguyen");
        // wifi_disconnect_device("wlu1u3i2");
        LOG_INFO("#############################################");
/******************************************************************************/

        if (w->type == REMOTE_WORK) {
            LOG_TRACE("Task done: %d", ((cmd_data_t *)w->data)->opcode);
        }
        delete_work(w);
    };

    LOG_INFO("Task handler thread exiting...");
    return NULL;
}
