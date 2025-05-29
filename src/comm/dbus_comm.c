#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/epoll.h>
#include <signal.h>
#include <sys/eventfd.h>

#include <log.h>
#include <sys_mgr.h>
#include <sys_comm.h>
#include <workqueue.h>

extern volatile sig_atomic_t g_run;
extern int event_fd;

void print_indent(int level) {
    for (int i = 0; i < level; i++) printf("  ");
}

void print_basic_type(DBusMessageIter* iter, int type, int indent) {
    print_indent(indent);
    switch (type) {
        case DBUS_TYPE_INT16: {
            int16_t val; dbus_message_iter_get_basic(iter, &val);
            printf("INT16: %d\n", val); break;
        }
        case DBUS_TYPE_INT32: {
            int32_t val; dbus_message_iter_get_basic(iter, &val);
            printf("INT32: %d\n", val); break;
        }
        case DBUS_TYPE_INT64: {
            int64_t val; dbus_message_iter_get_basic(iter, &val);
            printf("INT64: %lld\n", (long long)val); break;
        }
        case DBUS_TYPE_UINT64: {
            uint64_t val; dbus_message_iter_get_basic(iter, &val);
            printf("UINT64: %llu\n", (unsigned long long)val); break;
        }
        case DBUS_TYPE_BOOLEAN: {
            dbus_bool_t val; dbus_message_iter_get_basic(iter, &val);
            printf("BOOLEAN: %s\n", val ? "true" : "false"); break;
        }
        case DBUS_TYPE_DOUBLE: {
            double val; dbus_message_iter_get_basic(iter, &val);
            printf("DOUBLE: %.2f\n", val); break;
        }
        case DBUS_TYPE_STRING: {
            char* val; dbus_message_iter_get_basic(iter, &val);
            printf("STRING: %s\n", val); break;
        }
        case DBUS_TYPE_OBJECT_PATH: {
            char* path; dbus_message_iter_get_basic(iter, &path);
            printf("OBJECT_PATH: %s\n", path); break;
        }
        case DBUS_TYPE_SIGNATURE: {
            char* sig; dbus_message_iter_get_basic(iter, &sig);
            printf("SIGNATURE: %s\n", sig); break;
        }
        default:
            printf("Unhandled basic type: %c\n", type);
            break;
    }
}

void handle_struct(DBusMessageIter* iter, int indent) {
    DBusMessageIter sub;
    dbus_message_iter_recurse(iter, &sub);
    print_indent(indent); printf("STRUCT {\n");
    parse_dbus_iter(&sub, indent + 1);
    print_indent(indent); printf("}\n");
}

void handle_variant(DBusMessageIter* iter, int indent) {
    DBusMessageIter sub;
    dbus_message_iter_recurse(iter, &sub);
    print_indent(indent); printf("VARIANT (\n");
    parse_dbus_iter(&sub, indent + 1);
    print_indent(indent); printf(")\n");
}

void handle_array(DBusMessageIter* iter, int indent) {
    DBusMessageIter sub;
    dbus_message_iter_recurse(iter, &sub);
    int elem_type = dbus_message_iter_get_arg_type(&sub);

    // Check if array of dict entries
    if (elem_type == DBUS_TYPE_DICT_ENTRY) {
        print_indent(indent); printf("DICT {\n");
        while (dbus_message_iter_get_arg_type(&sub) == DBUS_TYPE_DICT_ENTRY) {
            DBusMessageIter entry;
            dbus_message_iter_recurse(&sub, &entry);
            print_indent(indent + 1); printf("KEY-VALUE:\n");
            parse_dbus_iter(&entry, indent + 2);
            dbus_message_iter_next(&sub);
        }
        print_indent(indent); printf("}\n");
    } else {
        print_indent(indent); printf("ARRAY of type '%c' {\n", elem_type);
        while (dbus_message_iter_get_arg_type(&sub) != DBUS_TYPE_INVALID) {
            parse_dbus_iter(&sub, indent + 1);
            dbus_message_iter_next(&sub);
        }
        print_indent(indent); printf("}\n");
    }
}

void parse_dbus_iter(DBusMessageIter* iter, int indent) {
    while (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_INVALID) {
        int type = dbus_message_iter_get_arg_type(iter);

        if (dbus_type_is_basic(type)) {
            print_basic_type(iter, type, indent);
        } else {
            switch (type) {
                case DBUS_TYPE_STRUCT:
                    handle_struct(iter, indent); break;
                case DBUS_TYPE_ARRAY:
                    handle_array(iter, indent); break;
                case DBUS_TYPE_VARIANT:
                    handle_variant(iter, indent); break;
                default:
                    print_indent(indent);
                    printf("Unhandled container type: %c\n", type);
            }
        }

        dbus_message_iter_next(iter);
    }
}

void handle_message(DBusMessage* msg) {
    DBusMessageIter args;

    if (!dbus_message_iter_init(msg, &args)) {
        printf("Message has no arguments.\n");
        return;
    }

    printf("=== Start Parsing D-Bus Message ===\n");
    parse_dbus_iter(&args, 0);
    printf("=== End Parsing ===\n");
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
    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
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
    DBusMessage *msg;

    int dbus_fd;
    dbus_connection_get_unix_fd(conn, &dbus_fd);
    if (dbus_fd < 0) {
        LOG_ERROR("Failed to get dbus fd");
        return NULL;
    }
 
    int epoll_fd;
    struct epoll_event ev, events[10];
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        LOG_ERROR("Failed to create epoll fd");
        return NULL;
    }
    ev.events = EPOLLIN;
    ev.data.fd = dbus_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, dbus_fd, &ev) == -1) {
        LOG_ERROR("Failed to add fd to epoll_ctl");
        close(epoll_fd);
        return NULL;
    }


    struct epoll_event ev2;
    ev2.events = EPOLLIN;
    ev2.data.fd = event_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event_fd, &ev2);

    LOG_INFO("System manager DBus communication is running...");
    while (g_run) {
        LOG_DEBUG("Waiting for DBus message...");
        int n = epoll_wait(epoll_fd, events, 10, -1);
        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == dbus_fd) {
                while (dbus_connection_read_write_dispatch(conn, 0)) {
                    msg = dbus_connection_pop_message(conn);
                    if (msg == NULL) {
                        usleep(10000);
                        continue;
                    }

                    if (dbus_message_is_method_call(msg, \
                                                    SYS_MGR_DBUS_IFACE, \
                                                    SYS_MGR_DBUS_METH)) {
			handle_message(msg);

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
			handle_message(msg);

                        work_t *w = malloc(sizeof(work_t));
                        w->opcode = i++;
                        snprintf(w->data, sizeof(w->data), "Message #%d", w->opcode);

                        push_work(w);
                        LOG_DEBUG("[DBus Thread] Pushed job %d", w->opcode);
                    }

                    dbus_message_unref(msg);
                    break;


                }
            } else if (events[i].data.fd == event_fd) {
                LOG_INFO("Received event ID [%lld], stopping DBus listener...", \
                       event_get(event_fd));
            }
        }
    }

    close(epoll_fd);
    LOG_INFO("The DBus handler thread in System Manager exited successfully");

    return NULL;
}
