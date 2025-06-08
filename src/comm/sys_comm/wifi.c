#include <stdio.h>

#include <glib.h>
#include <NetworkManager.h>

#include <log.h>
#include <sys_comm.h>

int scan_wifi()
{
    g_autoptr(GError) error = NULL;
    g_autoptr(NMClient) client = nm_client_new(NULL, &error);

    if (!client) {
        LOG_ERROR("Failed to create NMClient: %s\n", error->message);
        return EXIT_FAILURE;
    }

    const GPtrArray *devices = nm_client_get_devices(client);

    for (guint i = 0; i < devices->len; ++i) {
        NMDevice *device = g_ptr_array_index(devices, i);

        if (NM_IS_DEVICE_WIFI(device)) {
            NMDeviceWifi *wifi_device = NM_DEVICE_WIFI(device);

            LOG_TRACE("Found WiFi device: %s\n", nm_device_get_iface(device));

            // Optional: request scan
            nm_device_wifi_request_scan(wifi_device, NULL, &error);

            // Wait a bit for scan to complete
            g_usleep(5000000); // 2 seconds

            const GPtrArray *aps = nm_device_wifi_get_access_points(wifi_device);

            LOG_TRACE("Found %u access points:\n", aps->len);

            for (guint j = 0; j < aps->len; ++j) {
                NMAccessPoint *ap = g_ptr_array_index(aps, j);

                const guint8 *ssid = nm_access_point_get_ssid(ap);

                char *ssid_str; // = nm_utils_ssid_to_utf8(ssid, 100);
                if (ssid) {
                    ssid_str = nm_utils_ssid_to_utf8(g_bytes_get_data(ssid, NULL), g_bytes_get_size(ssid));
                }
                // char *ssid_str = nm_utils_ssid_to_utf8(ssid, 100);

                int strength = nm_access_point_get_strength(ap);
                NM80211ApSecurityFlags sec_flags = nm_access_point_get_flags(ap);

                LOG_TRACE("  SSID: %-30s Strength: %3d%% SecurityFlags: 0x%x\n",
                        ssid_str ? ssid_str : "<hidden>",
                        strength,
                        sec_flags);

                // g_free(ssid_str);
            }
        }
    }

    return EXIT_SUCCESS;
}
