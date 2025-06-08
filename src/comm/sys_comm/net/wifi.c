#include <stdio.h>
#include <glib.h>
#include <NetworkManager.h>

#include <log.h>
#include <sys_comm.h>

int wifi_scan_and_get_results(const char *iface, int scan)
{
    char *ssid_str = NULL;
    NMAccessPoint *ap = NULL;
    const guint8 *ssid = NULL;
    g_autoptr(GError) error = NULL;

    NMDevice *dev = g_nm_device_get_by_iface(iface);
    if (!NM_IS_DEVICE_WIFI(dev)) {
        LOG_ERROR("Wrong device type: %s\n", nm_device_get_iface(dev));
        return EXIT_FAILURE;
    } else {
        LOG_DEBUG("WiFi device: %s\n", nm_device_get_iface(dev));
    }

    NMDeviceWifi *wifi_device = NM_DEVICE_WIFI(dev);

    if (scan) {
        nm_device_wifi_request_scan(wifi_device, NULL, &error);
        g_usleep(3000000);
    }

    const GPtrArray *aps = nm_device_wifi_get_access_points(wifi_device);
    LOG_TRACE("Found %u access points:\n", aps->len);

    for (guint j = 0; j < aps->len; ++j) {
        ap = g_ptr_array_index(aps, j);
        ssid = nm_access_point_get_ssid(ap);
        if (!ssid) {
            continue;
        }

        ssid_str = nm_utils_ssid_to_utf8(g_bytes_get_data(ssid, NULL), \
                                         g_bytes_get_size(ssid));

        int strength = nm_access_point_get_strength(ap);
        NM80211ApSecurityFlags sec_flags = nm_access_point_get_flags(ap);

        LOG_TRACE("SSID: %-30s Strength: %3d%% SecurityFlags: 0x%x",
                ssid_str ? ssid_str : "<hidden>",
                strength,
                sec_flags);

        g_free(ssid_str);
    }

    return EXIT_SUCCESS;
}

int wifi_connected_ap_get(const char *iface)
{
    char *ssid_str = NULL;
    const guint8 *ssid = NULL;
    NMDeviceWifi *wifi_dev = NULL;
    NMAccessPoint *active_ap = NULL;
    NMDevice *dev = g_nm_device_get_by_iface(iface);

    if (!NM_IS_DEVICE_WIFI(dev)) {
        LOG_ERROR("Wrong device type: %s\n", nm_device_get_iface(dev));
        return EXIT_FAILURE;
    } else {
        LOG_DEBUG("WiFi device: %s\n", nm_device_get_iface(dev));
    }

    wifi_dev = NM_DEVICE_WIFI(dev);
    active_ap = nm_device_wifi_get_active_access_point(wifi_dev);
    if (active_ap) {
        ssid = nm_access_point_get_ssid(active_ap);
        if (!ssid) {
            return EXIT_FAILURE;
        }
        ssid_str = nm_utils_ssid_to_utf8(g_bytes_get_data(ssid, NULL), \
                                             g_bytes_get_size(ssid));
        LOG_TRACE("Wi-Fi interface [%s] connected to AP: [%s]\n", iface, \
                ssid_str ? ssid_str : "<hidden>");
        g_free(ssid_str);
    } else {
        LOG_TRACE("Wi-Fi interface [%s] is not connected to any AP", iface);
    }

    return EXIT_SUCCESS;
}

