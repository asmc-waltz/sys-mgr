#include <dbus/dbus.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#define SERVICE_NAME "com.example.DBusService"
#define OBJECT_PATH "/com/example/Service"
#define INTERFACE_NAME "com.example.Interface"

volatile sig_atomic_t g_run = 1;
int event_fd;

int event_trigger(int evfd, uint64_t code)
{
    uint64_t val = code;
    write(evfd, &val, sizeof(val));
}

int event_get(int evfd)
{
    uint64_t val;
    read(evfd, &val, sizeof(uint64_t));
}

void sig_handler(int sig) {
    switch (sig) {
        case SIGINT:
            printf("\n[+] Received SIGINT (Ctrl+C). Exiting...\n");
            event_trigger(event_fd, SIGINT);
            break;
        case SIGTERM:
            printf("\n[+] Received SIGTERM. Shutdown...\n");
            exit(0);
        case SIGABRT:
            printf("\n[+] Received SIGABRT. Exiting...\n");
            event_trigger(event_fd, SIGABRT);
            break;
        case SIGHUP:
            printf("\n[+] Received SIGHUP (terminal close). Exit...\n");
            exit(0);
        case SIGQUIT:
            printf("\n[+] Received SIGQUIT (Ctrl+\\). Exit...\n");
            exit(0);
        case SIGTSTP:
            printf("\n[+] Received SIGTSTP (Ctrl+Z). Ignore this signal.\n");
            break;
        default:
            printf("\n[!] Received unidentified signal: %d\n", sig);
            break;
    }
}

int setup_signal_handler()
{
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        perror("Error registering signal SIGINT handler");
        return SIG_ERR;
    }

    if (signal(SIGTERM, sig_handler) == SIG_ERR) {
        perror("Error registering signal SIGTERM handler");
        return SIG_ERR;
    }

    if (signal(SIGABRT, sig_handler) == SIG_ERR) {
        perror("Error registering signal SIGABRT handler");
        return SIG_ERR;
    }

    if (signal(SIGHUP, sig_handler) == SIG_ERR) {
        perror("Error registering signal SIGHUP handler");
        return SIG_ERR;
    }

    if (signal(SIGQUIT, sig_handler) == SIG_ERR) {
        perror("Error registering signal SIGQUIT handler");
        return SIG_ERR;
    }

    if (signal(SIGTSTP, sig_handler) == SIG_ERR) {
        perror("Error registering signal SIGTSTP handler");
        return SIG_ERR;
    }

    return 0;
}

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

void* listen_thread(void* arg) {
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

                    if (dbus_message_is_method_call(msg, INTERFACE_NAME, "TestMethod")) {
                        printf("    METHOD\n");
                        print_message(msg);
                        DBusMessage *reply;
                        DBusMessageIter args;
                        reply = dbus_message_new_method_return(msg);
                        dbus_message_iter_init_append(reply, &args);
                        const char *reply_str = "Method reply OK";
                        dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &reply_str);
                        dbus_connection_send(conn, reply, NULL);
                        dbus_message_unref(reply);
                    } else if (dbus_message_is_signal(msg, INTERFACE_NAME, "TestSignal")) {
                        printf("    SIGNAL\n");
                        print_message(msg);
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

    ret = dbus_bus_request_name(conn, SERVICE_NAME, DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Dbus request name error: %s\n", err.message);
        dbus_error_free(&err);
    }

    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        return NULL;
    }


    const char *match_rule = "type='signal',interface='com.example.Interface'";

    ret = add_dbus_match_rule(conn, match_rule);
    if (ret) {
        return NULL;
    }

    return conn;
}


int sys_mgr_work_cycle()
{
    printf("System manager service is running...\n");
    while (g_run) {
        usleep(200000);
        // printf("System manager walking cycle...\n");
    };

    printf("System manager is exiting...\n");
    return 0;
}



int main() {
    DBusConnection *conn;
    pthread_t dbus_listener;
    int ret = 0;

    if (setup_signal_handler()) {
        return SIG_ERR;
    }

    conn = setup_dbus();
    if (!conn) {
        printf("System Manager: Unable to establish connection with DBus\n");
        return -1;
    }

    // Prepare eventfd to notify epoll when communicating with a thread
    event_fd = eventfd(0, EFD_NONBLOCK);

    // This thread processes DBus messages for the system manager
    pthread_create(&dbus_listener, NULL, listen_thread, conn);

    // System manager's primary tasks are executed within a loop
    ret = sys_mgr_work_cycle();
    if (ret) {
        event_trigger(event_fd, SIGUSR1);
    }

    pthread_join(dbus_listener, NULL);

    close(event_fd);
    printf("All services stopped. Safe exit.\n");
    return 0;
}
