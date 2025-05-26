#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <log.h>
#include <sys_comm.h>

int send_method_call(DBusConnection* conn) {
    DBusMessage *msg;
    DBusMessageIter args;
    DBusPendingCall *pending;

    msg = dbus_message_new_method_call(SYS_MGR_DBUS_SER, \
                                       SYS_MGR_DBUS_OBJ_PATH, \
                                       SYS_MGR_DBUS_IFACE, \
                                       SYS_MGR_DBUS_METH);

    dbus_message_iter_init_append(msg, &args);

    const char *str = "Hello from client!";
    int num = 123;
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &str);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &num);

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
    DBusMessageIter args;

    msg = dbus_message_new_signal(UI_DBUS_OBJ_PATH, \
                                  UI_DBUS_IFACE, \
                                  UI_DBUS_SIG);

    dbus_message_iter_init_append(msg, &args);

    const char* signal_msg = "This is a signal!";
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &signal_msg);

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
    conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);

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
