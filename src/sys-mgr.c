// receiver.c
#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #include <time.h>
#include <sys/time.h>

void listen() {
    DBusMessage* msg;
    DBusMessageIter args;
    DBusConnection* conn;
    DBusError err;


    // time_t t;
    // struct tm *tm_info;
    struct timeval tv;


    dbus_error_init(&err);

    conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err)) { fprintf(stderr, "Connection Error (%s)\n", err.message); dbus_error_free(&err); }

    int ret = dbus_bus_request_name(conn, "org.example.Receiver", DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
    if (dbus_error_is_set(&err)) { fprintf(stderr, "Name Error (%s)\n", err.message); dbus_error_free(&err); }
    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) exit(1);

    printf("Waiting for messages...\n");

    while (1) {
        dbus_connection_read_write(conn, 0);
        msg = dbus_connection_pop_message(conn);

        if (msg == NULL) {
            usleep(100000);
            continue;
        }

        if (dbus_message_is_method_call(msg, "org.example.Receiver", "ReceiveData")) {
            printf("Received ReceiveData method call\n");

            if (!dbus_message_iter_init(msg, &args)) {
                fprintf(stderr, "Message has no arguments\n");
            } else if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_ARRAY) {
                fprintf(stderr, "Argument is not a dict (array)\n");
            } else {
                DBusMessageIter dict;
                dbus_message_iter_recurse(&args, &dict);
                while (dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_DICT_ENTRY) {
                    DBusMessageIter entry;
                    dbus_message_iter_recurse(&dict, &entry);

                    char* key;
                    dbus_message_iter_get_basic(&entry, &key);
                    dbus_message_iter_next(&entry); // move to value

                    DBusMessageIter variant;
                    dbus_message_iter_recurse(&entry, &variant);

                    int type = dbus_message_iter_get_arg_type(&variant);
                    printf("  Key: %s - ", key);
                    if (type == DBUS_TYPE_STRING) {
                        char* val;
                        dbus_message_iter_get_basic(&variant, &val);
                        printf("String Value: %s\n", val);
                    } else if (type == DBUS_TYPE_BOOLEAN) {
                        dbus_bool_t val;
                        dbus_message_iter_get_basic(&variant, &val);
                        printf("Boolean Value: %s\n", val ? "true" : "false");
                    } else {
                        printf("Other type: %c\n", type);
                    }

                    dbus_message_iter_next(&dict);
                }
            }

            // TIME

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
            gettimeofday(&tv, NULL);
            printf("Current time: %ld.%06ld seconds since the Epoch\n", tv.tv_sec, tv.tv_usec);

        }

        dbus_message_unref(msg);
    }
}

int main() {
    listen();
    return 0;
}
