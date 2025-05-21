#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>

// #include <time.h>
#include <sys/time.h>

void send_message() {
    DBusError err;
    DBusConnection* conn;
    DBusMessage* msg;
    DBusMessageIter args, dict, entry;
    dbus_uint32_t serial = 0;

    dbus_error_init(&err);

    conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err)) { fprintf(stderr, "Connection Error (%s)\n", err.message); dbus_error_free(&err); }

    if (!conn) { exit(1); }

    msg = dbus_message_new_method_call("org.example.Receiver",
                                       "/org/example/Receiver",
                                       "org.example.Receiver",
                                       "ReceiveData");

    dbus_message_iter_init_append(msg, &args);

    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &dict);

    // Add string entry
    dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, NULL, &entry);
    const char* key1 = "name";
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key1);
    DBusMessageIter variant1;
    dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "s", &variant1);
    const char* value1 = "Sensor A";
    dbus_message_iter_append_basic(&variant1, DBUS_TYPE_STRING, &value1);
    dbus_message_iter_close_container(&entry, &variant1);
    dbus_message_iter_close_container(&dict, &entry);

    // Add boolean entry
    dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, NULL, &entry);
    const char* key2 = "active";
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key2);
    DBusMessageIter variant2;
    dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "b", &variant2);
    dbus_bool_t value2 = TRUE;
    dbus_message_iter_append_basic(&variant2, DBUS_TYPE_BOOLEAN, &value2);
    dbus_message_iter_close_container(&entry, &variant2);
    dbus_message_iter_close_container(&dict, &entry);

    dbus_message_iter_close_container(&args, &dict);

    // Send
    if (!dbus_connection_send(conn, msg, &serial)) {
        fprintf(stderr, "Out Of Memory!\n");
        exit(1);
    }
    dbus_connection_flush(conn);
    printf("Message sent with serial %d\n", serial);

    dbus_message_unref(msg);


    // TIME
    // time_t t;
    // struct tm *tm_info;
    //
    // // Lấy thời gian hiện tại
    // time(&t);
    //
    // // Chuyển thành dạng cấu trúc thời gian địa phương
    // tm_info = localtime(&t);
    //
    // // In ra thời gian theo định dạng chuẩn: YYYY-MM-DD HH:MM:SS
    // printf("Current time: %04d-%02d-%02d %02d:%02d:%02d\n",
    //     tm_info->tm_year + 1900,
    //     tm_info->tm_mon + 1,
    //     tm_info->tm_mday,
    //     tm_info->tm_hour,
    //     tm_info->tm_min,
    //     tm_info->tm_sec
    // );

    struct timeval tv;
    gettimeofday(&tv, NULL);
    printf("Current time: %ld.%06ld seconds since the Epoch\n", tv.tv_sec, tv.tv_usec);

}

int main() {
    send_message();
    return 0;
}
