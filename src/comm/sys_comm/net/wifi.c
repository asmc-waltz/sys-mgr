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
        g_usleep(10000000);
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


static void
add_and_activate_cb(GObject *client, GAsyncResult *result, gpointer user_data)
{
    LOG_TRACE("Wi-Fi interface is not connected to any AP #########");
}


int wifi_connect_to_ssid(const char *iface_name, const char *ssid, \
                         const char *password)
{

    NMClient *client = get_nm_client();
    char *ssid_str = NULL;

    NMAccessPoint *ap = NULL;
    const guint8 *cur_ssid = NULL;
    NMDevice *dev = g_nm_device_get_by_iface(iface_name);
    NMDeviceWifi *wifi_dev = NM_DEVICE_WIFI(dev);

    // 2. Quét access point đang thấy
    const GPtrArray *aps = nm_device_wifi_get_access_points(wifi_dev);
    NMAccessPoint *target_ap = NULL;

    LOG_TRACE("Found %u access points:\n", aps->len);


    for (guint i = 0; i < aps->len; ++i) {
        NMAccessPoint *ap = g_ptr_array_index(aps, i);


        cur_ssid = nm_access_point_get_ssid(ap);
        if (!cur_ssid) {
            continue;
        }
        ssid_str = nm_utils_ssid_to_utf8(g_bytes_get_data(cur_ssid, NULL), \
                                             g_bytes_get_size(cur_ssid));
        LOG_TRACE("Wi-Fi interface [%s] can see AP: [%s]\n", iface_name, \
                ssid_str ? ssid_str : "<hidden>");



        // const char *ap_ssid = nm_utils_ssid_to_utf8(nm_access_point_get_ssid(ap));

        if (g_strcmp0(ssid_str, ssid) == 0) {

            LOG_TRACE("Wi-Fi interface [%s] connecting to AP: [%s]\n", iface_name, \
                ssid_str ? ssid_str : "<hidden>");
            g_free(ssid_str);
            target_ap = ap;
            break;
        }
        g_free(ssid_str);
    }

    if (!target_ap) {
        g_warning("SSID '%s' not found by interface %s", ssid, iface_name);
        return;
    }


  // 3. Tạo connection
    NMConnection *connection = nm_simple_connection_new();

    NMSettingWireless *s_wifi = (NMSettingWireless *)nm_setting_wireless_new();
    g_object_set(G_OBJECT(s_wifi),
                 NM_SETTING_WIRELESS_SSID, nm_access_point_get_ssid(target_ap),
                 NM_SETTING_WIRELESS_MODE, "infrastructure",
                 NULL);
    nm_connection_add_setting(connection, NM_SETTING(s_wifi));

    if (password && strlen(password) > 0) {
        NMSettingWirelessSecurity *s_sec = (NMSettingWirelessSecurity *)nm_setting_wireless_security_new();
        g_object_set(G_OBJECT(s_sec),
                     NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "wpa-psk",
                     NM_SETTING_WIRELESS_SECURITY_PSK, password,
                     NULL);
        nm_connection_add_setting(connection, NM_SETTING(s_sec));
    }

    NMSettingIPConfig *s_ip4 = (NMSettingIPConfig *)nm_setting_ip4_config_new();
    g_object_set(G_OBJECT(s_ip4),
                 NM_SETTING_IP_CONFIG_METHOD, "auto",
                 NULL);
    nm_connection_add_setting(connection, NM_SETTING(s_ip4));

    NMSettingConnection *s_con = (NMSettingConnection *)nm_setting_connection_new();
    g_object_set(G_OBJECT(s_con),
                 NM_SETTING_CONNECTION_TYPE, NM_SETTING_WIRELESS_SETTING_NAME,
                 NM_SETTING_CONNECTION_INTERFACE_NAME, iface_name,
                 NM_SETTING_CONNECTION_ID, ssid,
                 NULL);
    nm_connection_add_setting(connection, NM_SETTING(s_con));

    // 4. Add and activate connection (version 2)
    GVariant *options = NULL;  // nếu cần có thể set thêm options
    GError *error = NULL;
    NMActiveConnection *active_conn = NULL;

    nm_client_add_and_activate_connection2(client,
                                                         connection,
                                                         NM_DEVICE(wifi_dev),
                                                         NULL,  // specific AP optional
                                                         options,
                                                         NULL,
                                                         add_and_activate_cb,
                                                         &error);

    if (error) {
        g_warning("Failed to activate connection: %s", error->message);
        g_error_free(error);
    } else {
        // g_print("Connection to SSID '%s' initiated on %s (ActiveConnection=%p)\n",
        //         ssid, iface_name, active_conn);

        // Optionally: connect signal to track state of ActiveConnection
        // g_signal_connect(active_conn, "notify::state", G_CALLBACK(on_active_connection_state_changed), NULL);
    }

    // if (active_conn) {
    //     g_object_unref(active_conn);
    // }
}
