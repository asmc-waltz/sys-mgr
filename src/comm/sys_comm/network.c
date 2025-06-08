#include <stdio.h>
#include <glib.h>
#include <NetworkManager.h>

#include <log.h>
#include <sys_comm.h>

NMClient *nm_client = NULL;

NMClient * get_nm_client()
{
    return nm_client;
}

void set_nm_client( NMClient *client)
{
    // Mutex ?
    nm_client = client;
}

int network_manager_comm_init()
{
    g_autoptr(GError) error = NULL;
    NMClient *client = NULL;

    client = nm_client_new(NULL, &error);
    if (!client) {
        LOG_ERROR("Failed to create NMClient: %s\n", error->message);
        return EXIT_FAILURE;
    }
    set_nm_client(client);

    return EXIT_SUCCESS;
}

void network_manager_comm_deinit()
{
    NMClient *client = get_nm_client();

    if (client) {
        g_object_unref(client);
        set_nm_client(NULL);
    }
}


