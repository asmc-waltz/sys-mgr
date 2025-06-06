#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <log.h>
#include <sys_mgr.h>
#include <workqueue.h>


#include <stdio.h>
#include <glib.h>
#include <NetworkManager.h>

extern volatile sig_atomic_t g_run;

int scan_wifi()
{
    g_autoptr(GError) error = NULL;
    g_autoptr(NMClient) client = nm_client_new(NULL, &error);

    if (!client) {
        g_printerr("Failed to create NMClient: %s\n", error->message);
        return EXIT_FAILURE;
    }

    const GPtrArray *devices = nm_client_get_devices(client);

    for (guint i = 0; i < devices->len; ++i) {
        NMDevice *device = g_ptr_array_index(devices, i);

        if (NM_IS_DEVICE_WIFI(device)) {
            NMDeviceWifi *wifi_device = NM_DEVICE_WIFI(device);

            g_print("Found WiFi device: %s\n", nm_device_get_iface(device));

            // Optional: request scan
            nm_device_wifi_request_scan(wifi_device, NULL, &error);

            // Wait a bit for scan to complete
            g_usleep(2000000); // 2 seconds

            const GPtrArray *aps = nm_device_wifi_get_access_points(wifi_device);

            g_print("Found %u access points:\n", aps->len);

            for (guint j = 0; j < aps->len; ++j) {
                NMAccessPoint *ap = g_ptr_array_index(aps, j);

                const guint8 *ssid = nm_access_point_get_ssid(ap);
                char *ssid_str = nm_utils_ssid_to_utf8(ssid, 100);

                int strength = nm_access_point_get_strength(ap);
                NM80211ApSecurityFlags sec_flags = nm_access_point_get_flags(ap);

                g_print("  SSID: %-30s Strength: %3d%% SecurityFlags: 0x%x\n",
                        ssid_str ? ssid_str : "<hidden>",
                        strength,
                        sec_flags);

                g_free(ssid_str);
            }
        }
    }

    return EXIT_SUCCESS;
}

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



        scan_wifi();



        sleep(1);
        LOG_TRACE("Task done: %s", w->data);

        free(w);
    };

    LOG_INFO("Task handler thread exiting...");
    return NULL;
}
