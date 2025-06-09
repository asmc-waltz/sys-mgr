#include <stdio.h>
#include <glib.h>
#include <glib-object.h>
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


NMAccessPoint * find_ap_on_wifi_device(NMDevice *device, const char *bssid, \
                                       const char *ssid, gboolean complete)
{
    const GPtrArray *aps;
    NMAccessPoint *ap = NULL;
    int i;

    g_return_val_if_fail(NM_IS_DEVICE_WIFI(device), NULL);

    aps = nm_device_wifi_get_access_points(NM_DEVICE_WIFI(device));
    LOG_TRACE("Found %u access points:\n", aps->len);

    for (i = 0; i < aps->len; i++) {
        NMAccessPoint *candidate_ap = g_ptr_array_index(aps, i);

        // Match BSSID if requested
        if (bssid) {
            const char *candidate_bssid = \
                nm_access_point_get_bssid(candidate_ap);
            if (!candidate_bssid)
                continue;

            if (complete) {
                if (g_str_has_prefix(candidate_bssid, bssid))
                    LOG_TRACE("%s\n", candidate_bssid);
            } else if (strcmp(bssid, candidate_bssid) != 0)
                continue;
        }

        // Match SSID if requested
        if (ssid) {
            GBytes *candidate_ssid = nm_access_point_get_ssid(candidate_ap);
            if (!candidate_ssid)
                continue;

            char *ssid_tmp = \
                nm_utils_ssid_to_utf8(g_bytes_get_data(candidate_ssid, NULL), \
                                        g_bytes_get_size(candidate_ssid));

            if (complete) {
                if (g_str_has_prefix(ssid_tmp, ssid))
                    LOG_TRACE("%s\n", ssid_tmp);
            } else if (strcmp(ssid, ssid_tmp) != 0) {
                g_free(ssid_tmp);
                continue;
            }

            LOG_TRACE("Wi-Fi interface [%s] found AP: [%s]\n", \
                      nm_device_get_iface(device), \
                      ssid_tmp ? ssid_tmp : "<hidden>");

            g_free(ssid_tmp);
        }

        // If not in "complete" mode, return first match
        if (!complete) {
            ap = candidate_ap;
            break;
        }
    }

    return ap;
}

NMConnection * find_connection_on_wifi_device(NMDevice *dev, \
                                              NMAccessPoint *ap, \
                                              const char *con_name)
{
    const GPtrArray *avail_cons;
    gboolean name_match = FALSE;
    NMConnection *connection = NULL;
    int i;

    g_return_val_if_fail(NM_IS_DEVICE_WIFI(dev), NULL);
    g_return_val_if_fail(NM_IS_ACCESS_POINT(ap), NULL);

    avail_cons = nm_device_get_available_connections(dev);
    LOG_TRACE("Found %u available connections\n", avail_cons->len);

    for (i = 0; i < avail_cons->len; i++) {
        NMConnection *avail_con = g_ptr_array_index(avail_cons, i);
        const char   *id = nm_connection_get_id(avail_con);

        LOG_TRACE("Wi-Fi connection ID found: [%s]", id);

        // Match connection name (optional)
        if (con_name) {
            if (!id || strcmp(id, con_name) != 0)
                continue;
            name_match = TRUE;
        }

        // Check if connection is valid for AP
        if (nm_access_point_connection_valid(ap, avail_con)) {
            connection = g_object_ref(avail_con);
            LOG_TRACE("Wi-Fi connection ID [%s] is valid for AP", id);
            break;
        }
    }

    if (name_match && !connection) {
        LOG_TRACE("Error: Connection '%s' exists but properties don't match.", con_name);
        return NULL;
    }

    return connection;
}

static void add_and_activate_cb(GObject *client, GAsyncResult *result, \
                                gpointer user_data)
{
    GError *error = NULL;
    GVariant *out_result = NULL;

    NMActiveConnection *ac = nm_client_add_and_activate_connection2_finish(\
                                NM_CLIENT(client), result, &out_result, &error);

    if (error) {
        LOG_ERROR("Failed to activate connection (async): %s", \
                  error->message);
        g_error_free(error);
        return;
    }

    LOG_INFO("Successfully activating connection (NMActiveConnection=%p)", ac);

    if (out_result) {
        gchar *out_str = g_variant_print(out_result, TRUE);
        LOG_TRACE("Out result: %s", out_str);
        g_free(out_str);
        g_variant_unref(out_result);
    }

    g_object_unref(ac);
}


int wifi_connect_to_ssid(const char *iface_name, const char *ssid, const char *password)
{
    NMClient *client = get_nm_client();
    if (!client) {
        LOG_ERROR("Failed to get NMClient");
        return EXIT_FAILURE;
    }

    NMDevice *dev = g_nm_device_get_by_iface(iface_name);
    if (!dev) {
        LOG_ERROR("Interface '%s' not found", iface_name);
        return EXIT_FAILURE;
    }

    NMAccessPoint *ap = find_ap_on_wifi_device(dev, NULL, ssid, FALSE);
    if (!ap) {
        LOG_ERROR("SSID '%s' not found by interface '%s'", ssid, iface_name);
        return EXIT_FAILURE;
    }

    LOG_INFO("Found target AP for SSID '%s' on interface '%s'", ssid, iface_name);

    NMConnection *connection = nm_simple_connection_new();

    NMSettingWireless *s_wifi = (NMSettingWireless *)nm_setting_wireless_new();
    g_object_set(G_OBJECT(s_wifi),
                 NM_SETTING_WIRELESS_SSID, nm_access_point_get_ssid(ap),
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

    GError *error = NULL;
    GVariant *options = NULL;
    nm_client_add_and_activate_connection2(client,
                                           connection,
                                           dev,
                                           NULL,
                                           options,
                                           NULL,
                                           add_and_activate_cb,
                                           &error);

    LOG_INFO("Initiated connection to SSID '%s' on interface '%s'", ssid, iface_name);

    // Release temporary refs
    g_object_unref(connection);

    return EXIT_SUCCESS;
}
