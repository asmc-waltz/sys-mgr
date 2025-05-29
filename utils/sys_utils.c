#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <log.h>
#include <sys_comm.h>

typedef struct {
    int length;
    int opcode;
    void *data;
} g_frame;

int send_method_call(DBusConnection* conn) {
    DBusMessage *msg;
    DBusMessageIter args;
    DBusPendingCall *pending;

    msg = dbus_message_new_method_call(SYS_MGR_DBUS_SER, \
                                       SYS_MGR_DBUS_OBJ_PATH, \
                                       SYS_MGR_DBUS_IFACE, \
                                       SYS_MGR_DBUS_METH);

    dbus_message_iter_init_append(msg, &args);

    const char *str = "TERMINAL_UI";
    int opcode = 0;
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &str);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &opcode);

    if (!dbus_connection_send_with_reply(conn, msg, &pending, -1)) {
        LOG_ERROR("Out of memory!");
        exit(1);
    }

    dbus_connection_flush(conn);
    dbus_message_unref(msg);

    dbus_pending_call_block(pending);

    DBusMessage *reply = dbus_pending_call_steal_reply(pending);
    dbus_pending_call_unref(pending);

    if (reply) {
        DBusMessageIter reply_args;
        if (dbus_message_iter_init(reply, &reply_args)) {
            char *reply_str;
            dbus_message_iter_get_basic(&reply_args, &reply_str);
            LOG_INFO("Method call reply: %s", reply_str);
        }
        dbus_message_unref(reply);
    }
}

int send_signal(DBusConnection* conn) {
    DBusMessage* msg;
    DBusMessageIter args, struct_iter, dict_iter, entry_iter;

    const char* name = "TERMINAL_UI";
    int32_t opcode = 23;
    const char* key1 = "SSID";
    const char* val1 = "local_network";
    const char* key2 = "PASSWORD";
    const char* val2 = "123456";
    const char* key3 = "AUTO_CONNECT";
    const char* val3 = "1";
    const char* extra = "private network";


    msg = dbus_message_new_signal(UI_DBUS_OBJ_PATH, \
                                  UI_DBUS_IFACE, \
                                  UI_DBUS_SIG);
    if (!msg) {
        fprintf(stderr, "Message NULL\n");
        exit(1);
    }

    dbus_message_iter_init_append(msg, &args);

    // Begin struct (si a{ss} s)
    dbus_message_iter_open_container(&args, DBUS_TYPE_STRUCT, NULL, &struct_iter);

    dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &name);
    dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_INT32, &opcode);

    // Begin dict
    dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "{ss}", &dict_iter);
    // Entry 1
    dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &entry_iter);
    dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &key1);
    dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &val1);
    dbus_message_iter_close_container(&dict_iter, &entry_iter);
    // Entry 2
    dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &entry_iter);
    dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &key2);
    dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &val2);
    dbus_message_iter_close_container(&dict_iter, &entry_iter);
    // Entry 3
    dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &entry_iter);
    dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &key3);
    dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &val3);
    dbus_message_iter_close_container(&dict_iter, &entry_iter);

    dbus_message_iter_close_container(&struct_iter, &dict_iter);

    // Extra string
    dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &extra);

    dbus_message_iter_close_container(&args, &struct_iter);

    if (!dbus_connection_send(conn, msg, NULL)) {
        LOG_ERROR("Out of memory!");
        exit(1);
    }

    dbus_connection_flush(conn);
    dbus_message_unref(msg);
    LOG_INFO("Signal sent.");
}

int main(int argc, char** argv) {
    DBusError err;
    DBusConnection* conn;
    int ret = 0;

    dbus_error_init(&err);
    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);

    if (dbus_error_is_set(&err)) {
        LOG_ERROR("Connection Error (%s)", err.message);
        dbus_error_free(&err);
    }

    if (!conn) exit(1);

    ret = dbus_bus_request_name(conn, \
                                UI_DBUS_SER, \
                                DBUS_NAME_FLAG_REPLACE_EXISTING, \
                                &err);
    if (dbus_error_is_set(&err)) {
        LOG_ERROR("Dbus request name error: %s", err.message);
        dbus_error_free(&err);
    }

    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        return NULL;
    }

    if (argc > 1 && strcmp(argv[1], "signal") == 0)
        send_signal(conn);
    else
        send_method_call(conn);

    return 0;
}
