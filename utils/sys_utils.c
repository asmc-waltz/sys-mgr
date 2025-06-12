#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <log.h>
#include <dbus_comm.h>

// Encode DataFrame into an existing DBusMessage
bool encode_data_frame(DBusMessage *msg, const DataFrame *frame)
{
    DBusMessageIter iter, array_iter, struct_iter, variant_iter;

    dbus_message_iter_init_append(msg, &iter);

    dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &frame->component_id);
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &frame->topic_id);
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &frame->opcode);

    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(siiv)", &array_iter);

    for (int i = 0; i < frame->entry_count; ++i) {
        PayloadEntry *entry = &frame->entries[i];

        dbus_message_iter_open_container(&array_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter);

        dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &entry->key);
        dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_INT32, &entry->data_type);
        dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_INT32, &entry->data_length);

        const char *sig = NULL;
        switch (entry->data_type) {
            case DBUS_TYPE_STRING: sig = "s"; break;
            case DBUS_TYPE_INT32:  sig = "i"; break;
            case DBUS_TYPE_UINT32: sig = "u"; break;
            case DBUS_TYPE_DOUBLE: sig = "d"; break;
            default:
                LOG_ERROR("Unsupported data_type %d for key '%s'", entry->data_type, entry->key);
                return false;
        }

        dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_VARIANT, sig, &variant_iter);

        switch (entry->data_type) {
            case DBUS_TYPE_STRING:
                dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &entry->value.str);
                break;
            case DBUS_TYPE_INT32:
                dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_INT32, &entry->value.i32);
                break;
            case DBUS_TYPE_UINT32:
                dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_UINT32, &entry->value.u32);
                break;
            case DBUS_TYPE_DOUBLE:
                dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_DOUBLE, &entry->value.dbl);
                break;
        }

        dbus_message_iter_close_container(&struct_iter, &variant_iter);
        dbus_message_iter_close_container(&array_iter, &struct_iter);
    }

    dbus_message_iter_close_container(&iter, &array_iter);
    return true;
}

// Decode DBusMessage into DataFrame
bool decode_data_frame(DBusMessage *msg, DataFrame *out)
{
    DBusMessageIter iter, array_iter, struct_iter, variant_iter;

    if (!dbus_message_iter_init(msg, &iter)) {
        LOG_ERROR("Failed to init DBus iterator");
        return false;
    }

    dbus_message_iter_get_basic(&iter, &out->component_id);
    dbus_message_iter_next(&iter);

    dbus_message_iter_get_basic(&iter, &out->topic_id);
    dbus_message_iter_next(&iter);

    dbus_message_iter_get_basic(&iter, &out->opcode);
    dbus_message_iter_next(&iter);

    dbus_message_iter_recurse(&iter, &array_iter);

    int i = 0;
    while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_STRUCT && i < MAX_ENTRIES) {
        PayloadEntry *entry = &out->entries[i];
        dbus_message_iter_recurse(&array_iter, &struct_iter);

        dbus_message_iter_get_basic(&struct_iter, &entry->key);
        dbus_message_iter_next(&struct_iter);

        dbus_message_iter_get_basic(&struct_iter, &entry->data_type);
        dbus_message_iter_next(&struct_iter);

        dbus_message_iter_get_basic(&struct_iter, &entry->data_length);
        dbus_message_iter_next(&struct_iter);

        dbus_message_iter_recurse(&struct_iter, &variant_iter);

        switch (entry->data_type) {
            case DBUS_TYPE_STRING:
                dbus_message_iter_get_basic(&variant_iter, &entry->value.str);
                break;
            case DBUS_TYPE_INT32:
                dbus_message_iter_get_basic(&variant_iter, &entry->value.i32);
                break;
            case DBUS_TYPE_UINT32:
                dbus_message_iter_get_basic(&variant_iter, &entry->value.u32);
                break;
            case DBUS_TYPE_DOUBLE:
                dbus_message_iter_get_basic(&variant_iter, &entry->value.dbl);
                break;
            default:
                LOG_WARN("Unsupported type %d for entry %d", entry->data_type, i);
                break;
        }

        dbus_message_iter_next(&array_iter);
        i++;
    }

    out->entry_count = i;
    return true;
}

void create_sample_frame(DataFrame *frame) {
    frame->component_id = "terminal-ui";
    frame->topic_id = 1001;
    frame->opcode = 42;

    frame->entry_count = 2;

    frame->entries[0].key = "username";
    frame->entries[0].data_type = DBUS_TYPE_STRING;
    frame->entries[0].data_length = 0;
    frame->entries[0].value.str = "alice";

    frame->entries[1].key = "age";
    frame->entries[1].data_type = DBUS_TYPE_INT32;
    frame->entries[1].data_length = 0;
    frame->entries[1].value.i32 = 30;
}


int send_method_call(DBusConnection* conn) {
    DBusMessage *msg;
    DBusMessageIter args;
    DBusPendingCall *pending;

    msg = dbus_message_new_method_call(SYS_MGR_DBUS_SER, \
                                       SYS_MGR_DBUS_OBJ_PATH, \
                                       SYS_MGR_DBUS_IFACE, \
                                       SYS_MGR_DBUS_METH);


    /////////////////////////////////////////////////
    DataFrame frame;
    create_sample_frame(&frame);

    if (!encode_data_frame(msg, &frame)) {
        LOG_ERROR("Failed to encode data frame");
        dbus_message_unref(msg);
        return EXIT_FAILURE;
    }

    /////////////////////////////////////////////////
        /*
    dbus_message_iter_init_append(msg, &args);

    const char *str = "TERMINAL_UI";
    int opcode = 0;
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &str);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &opcode);
        */
    /////////////////////////////////////////////////

    if (!dbus_connection_send_with_reply(conn, msg, &pending, -1)) {
        LOG_ERROR("Out of memory!");
        return EXIT_FAILURE;
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

    return EXIT_SUCCESS;
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
        return EXIT_FAILURE;
    }

    dbus_connection_flush(conn);
    dbus_message_unref(msg);
    LOG_INFO("Signal sent.");

    return EXIT_SUCCESS;
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

    if (!conn) {
        return EXIT_FAILURE;
    }

    ret = dbus_bus_request_name(conn, \
                                UI_DBUS_SER, \
                                DBUS_NAME_FLAG_REPLACE_EXISTING, \
                                &err);
    if (dbus_error_is_set(&err)) {
        LOG_ERROR("Dbus request name error: %s", err.message);
        dbus_error_free(&err);
    }

    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        return EXIT_FAILURE;
    }

    if (argc > 1 && strcmp(argv[1], "signal") == 0)
        send_signal(conn);
    else
        send_method_call(conn);

    return EXIT_SUCCESS;
}
