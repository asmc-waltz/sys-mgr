#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <inttypes.h>
#include <stdbool.h>

#include <dbus/dbus.h>

#include <log.h>
#include <sys_mgr.h>
#include <dbus_comm.h>
#include <sys_comm.h>
#include <workqueue.h>

#define MAX_EVENTS 2
extern volatile sig_atomic_t g_run;
extern int event_fd;

// Encode cmd_data_t into an existing DBusMessage
bool encode_data_frame(DBusMessage *msg, const cmd_data_t *cmd)
{
    DBusMessageIter iter, array_iter, struct_iter, variant_iter;

    dbus_message_iter_init_append(msg, &iter);

    dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &cmd->component_id);
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &cmd->topic_id);
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &cmd->opcode);

    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(siiv)", &array_iter);

    for (int i = 0; i < cmd->entry_count; ++i) {
        payload_t *entry = &cmd->entries[i];

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

// Decode DBusMessage into cmd_data_t
bool decode_data_frame(DBusMessage *msg, cmd_data_t *out)
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
        payload_t *entry = &out->entries[i];
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

cmd_data_t * handle_received_message(DBusMessage *msg) {
    cmd_data_t *cmd = NULL;

    cmd = (cmd_data_t *)calloc(1, sizeof(cmd_data_t));
    if (!cmd) {
        LOG_ERROR("Failed to allocate memory for command data");
        return NULL;
    }

    if (!decode_data_frame(msg, cmd)) {
        free(cmd);
        LOG_ERROR("Failed to decode received cmd_data_t");
        return NULL;
    }

    LOG_INFO("Received frame from component: %s", cmd->component_id);
    LOG_INFO("Topic: %d, Opcode: %d", cmd->topic_id, cmd->opcode);

    for (int i = 0; i < cmd->entry_count; ++i) {
        payload_t *entry = &cmd->entries[i];
        LOG_INFO("Key: %s, Type: %c", entry->key, entry->data_type);
        switch (entry->data_type) {
            case DBUS_TYPE_STRING:
                LOG_INFO("Value (string): %s", entry->value.str);
                break;
            case DBUS_TYPE_INT32:
                LOG_INFO("Value (int): %d", entry->value.i32);
                break;
            default:
                LOG_WARN("Unsupported type: %c", entry->data_type);
        }
    }

    return cmd;
}

int dbus_event_handler(DBusConnection *conn)
{
    DBusMessage *msg = NULL;
    cmd_data_t *cmd;
    work_t *w = NULL;

    while (dbus_connection_read_write_dispatch(conn, 0)) {
        msg = dbus_connection_pop_message(conn);
        if (msg == NULL) {
            usleep(10000);
            continue;
        }

        if (dbus_message_is_method_call(msg, \
                                        SYS_MGR_DBUS_IFACE, \
                                        SYS_MGR_DBUS_METH)) {
            cmd = handle_received_message(msg);
            // TODO: Handle cmd from message

            DBusMessage *reply;
            DBusMessageIter args;
            reply = dbus_message_new_method_return(msg);
            dbus_message_iter_init_append(reply, &args);
            const char *reply_str = "Method reply OK";
            dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &reply_str);
            dbus_connection_send(conn, reply, NULL);
            dbus_message_unref(reply);
        } else if (dbus_message_is_signal(msg, \
                                          UI_DBUS_IFACE, \
                                          UI_DBUS_SIG)) {
            cmd = handle_received_message(msg);
            if (!cmd) {
                LOG_ERROR("Unable to handle message");
            }

            w = create_work(cmd);

            // TODO: Handle cmd from message

            // work_t *w = malloc(sizeof(work_t));
            // w->opcode = 10;
            // snprintf(w->data, sizeof(w->data), "Message #%d", w->opcode);

            push_work(w);
            LOG_DEBUG("[DBus Thread] Pushed job %d", w->cmd->opcode);
        }

        dbus_message_unref(msg);
        break;
     }
}


int add_dbus_match_rule(DBusConnection *conn, const char *rule)
{
    DBusError err;
    if (NULL == conn) {
        return EINVAL;
    }

    LOG_INFO("Adds a match rule: [%s]", rule);
    dbus_error_init(&err);
    dbus_bus_add_match(conn, rule, &err);
    if (dbus_error_is_set(&err)) {
        LOG_ERROR("Add failed (error: %s)", err.message);
        dbus_error_free(&err);
        return EINVAL;
    }

    dbus_connection_flush(conn);
    LOG_TRACE("Add match rule succeeded");
    return 0;
}

DBusConnection * setup_dbus()
{
    DBusConnection *conn = NULL;
    DBusError err;
    int ret = 0;

    dbus_error_init(&err);
    conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err)) {
        LOG_ERROR("DBus connection Error: %s", err.message);
        dbus_error_free(&err);
    }

    if (NULL == conn) {
        return NULL;
    }

    ret = dbus_bus_request_name(conn, \
                                SYS_MGR_DBUS_SER, \
                                DBUS_NAME_FLAG_REPLACE_EXISTING, \
                                &err);
    if (dbus_error_is_set(&err)) {
        LOG_FATAL("Dbus request name error: %s", err.message);
        dbus_error_free(&err);
        return NULL;
    }

    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        return NULL;
    }

    char* match_rule = (char*)calloc(256, sizeof(char));
    if (!match_rule) {
        LOG_ERROR("Failed to allocate memory");
        return NULL;
    }
    sprintf(match_rule,
        "type='signal',interface='%s',member='%s',path='%s'",
        UI_DBUS_IFACE, UI_DBUS_SIG, UI_DBUS_OBJ_PATH);

    ret = add_dbus_match_rule(conn, match_rule);
    if (ret) {
        return NULL;
    }

    free(match_rule);

    return conn;
}

void* dbus_listen_thread(void* arg) {
    DBusConnection *conn = (DBusConnection*)arg;
    int dbus_fd;
    int epoll_fd;
    struct epoll_event ev;
    struct epoll_event events_detected[MAX_EVENTS];
    int n_ready;
    int ready_fd;

    // Get DBus file desc
    dbus_connection_get_unix_fd(conn, &dbus_fd);
    if (dbus_fd < 0) {
        LOG_ERROR("Failed to get dbus fd");
        return NULL;
    }
 
    // Create epoll file desc
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        LOG_ERROR("Failed to create epoll fd");
        return NULL;
    }

    ev.events = EPOLLIN;

    // Add DBus file desc
    ev.data.fd = dbus_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, dbus_fd, &ev) == -1) {
        LOG_ERROR("Failed to add fd to epoll_ctl");
        close(epoll_fd);
        return NULL;
    }

    // Add Event file desc
    ev.data.fd = event_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event_fd, &ev) == -1) {
        LOG_ERROR("Failed to add fd to epoll_ctl");
        close(epoll_fd);
        return NULL;
    }

    LOG_INFO("System manager DBus communication is running...");
    while (g_run) {
        LOG_DEBUG("[DBus]--> Waiting for next DBus message...");
        n_ready = epoll_wait(epoll_fd, events_detected, MAX_EVENTS, -1);
        for (int cnt = 0; cnt < n_ready; cnt++) {
            ready_fd = events_detected[cnt].data.fd;
            if (ready_fd == dbus_fd) {
                dbus_event_handler(conn);
            } else if (ready_fd == event_fd) {
                LOG_INFO("Received event ID [%" PRIu64 \
                                "], stopping DBus listener...", \
                                event_get(event_fd));
            }
        }
    }

    close(epoll_fd);
    LOG_INFO("The DBus handler thread in System Manager exited successfully");

    return NULL;
}
