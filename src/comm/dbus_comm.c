#include <stdio.h>
#include <errno.h>
#include <sys/epoll.h>
#include <signal.h>
#include <sys/eventfd.h>

#include <sys_mgr.h>
#include <sys_comm.h>
#include <workqueue.h>

extern volatile sig_atomic_t g_run;
extern int event_fd;

void print_message(DBusMessage *msg) {
    DBusMessageIter args;
    if (!dbus_message_iter_init(msg, &args)) {
        printf("Message has no arguments\n");
        return;
    }

    printf("Received message: %s\n", dbus_message_get_member(msg));

    do {
        int arg_type = dbus_message_iter_get_arg_type(&args);
        if (arg_type == DBUS_TYPE_INVALID) break;

        printf("  Arg type: %c\n", arg_type);
        if (arg_type == DBUS_TYPE_STRING) {
            char *val;
            dbus_message_iter_get_basic(&args, &val);
            printf("    STRING: %s\n", val);
        } else if (arg_type == DBUS_TYPE_INT32) {
            int val;
            dbus_message_iter_get_basic(&args, &val);
            printf("    INT32: %d\n", val);
        } else {
            printf("    Other type (not printed)\n");
        }

        dbus_message_iter_next(&args);
    } while (1);
}

int add_dbus_match_rule(DBusConnection *conn, const char *rule)
{
    DBusError err;
    if (NULL == conn) {
        return EINVAL;
    }

    printf("Adds a match rule: [%s]\n", rule);
    dbus_error_init(&err);
    dbus_bus_add_match(conn, rule, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "\tAdd failed (error: %s)\n", err.message);
        dbus_error_free(&err);
        return EINVAL;
    }

    dbus_connection_flush(conn);
    printf("\tAdd succeeded\n");
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
        fprintf(stderr, "Dbus connection Error: %s\n", err.message);
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
        fprintf(stderr, "Dbus request name error: %s\n", err.message);
        dbus_error_free(&err);
    }

    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        return NULL;
    }

    char* match_rule = (char*)calloc(256, sizeof(char));
    if (!match_rule) {
        printf("Failed to allocate memory\n");
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
        fprintf(stderr, "Failed to get dbus fd\n");
        return NULL;
    }
 
    int epoll_fd;
    struct epoll_event ev, events[10];
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        return NULL;
    }
    ev.events = EPOLLIN;
    ev.data.fd = dbus_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, dbus_fd, &ev) == -1) {
        perror("epoll_ctl");
        close(epoll_fd);
        return NULL;
    }


    struct epoll_event ev2;
    ev2.events = EPOLLIN;
    ev2.data.fd = event_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event_fd, &ev2);



    printf("System manager DBus communication is running...\n");
    while (g_run) {
        printf("Listening...\n");
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
                        print_message(msg);
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
                        print_message(msg);

                            work_t *w = malloc(sizeof(work_t));
                            w->opcode = i++;
                            snprintf(w->data, sizeof(w->data), "Message #%d", w->opcode);

                            push_work(w);
                            printf("[DBus Thread] Pushed job %d\n", w->opcode);
                    }

                    dbus_message_unref(msg);
                    break;


                }
            } else if (events[i].data.fd == event_fd) {
                printf("Received event ID [%lld], stopping DBus listener...\n", \
                       event_get(event_fd));
                g_run = 0;
            }
        }
    }

    close(epoll_fd);
    printf("The DBus handler thread in System Manager exited successfully\n");

    return NULL;
}
