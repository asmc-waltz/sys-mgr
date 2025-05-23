#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SERVICE_NAME "com.example.DBusService"
#define OBJECT_PATH "/com/example/Service"
#define INTERFACE_NAME "com.example.Interface"

void send_method_call(DBusConnection* conn) {
    DBusMessage *msg;
    DBusMessageIter args;
    DBusPendingCall *pending;

    msg = dbus_message_new_method_call(SERVICE_NAME, OBJECT_PATH, INTERFACE_NAME, "TestMethod");
    dbus_message_iter_init_append(msg, &args);

    const char *str = "Hello from client!";
    int num = 123;
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &str);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &num);

    if (!dbus_connection_send_with_reply(conn, msg, &pending, -1)) {
        fprintf(stderr, "Out of memory!\n");
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
            printf("Method call reply: %s\n", reply_str);
        }
        dbus_message_unref(reply);
    }
}

void send_signal(DBusConnection* conn) {
    DBusMessage* msg;
    DBusMessageIter args;

    msg = dbus_message_new_signal(OBJECT_PATH, INTERFACE_NAME, "TestSignal");

    dbus_message_iter_init_append(msg, &args);

    const char* signal_msg = "This is a signal!";
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &signal_msg);

    if (!dbus_connection_send(conn, msg, NULL)) {
        fprintf(stderr, "Out of memory!\n");
        exit(1);
    }

    dbus_connection_flush(conn);
    dbus_message_unref(msg);
    printf("Signal sent.\n");
}

int main(int argc, char** argv) {
    DBusError err;
    DBusConnection* conn;

    dbus_error_init(&err);
    conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);

    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Connection Error (%s)\n", err.message);
        dbus_error_free(&err);
    }

    if (!conn) exit(1);

    if (argc > 1 && strcmp(argv[1], "signal") == 0)
        send_signal(conn);
    else
        send_method_call(conn);

    return 0;
}
