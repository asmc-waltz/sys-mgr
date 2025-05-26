#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/epoll.h>
#include <dbus/dbus.h>

#include <log.h>
#include <sys_mgr.h>
#include <sys_comm.h>
#include <workqueue.h>

extern int event_fd;
volatile sig_atomic_t g_run = 1;

void sig_handler(int sig) {
    switch (sig) {
        case SIGINT:
            LOG_WARN("[+] Received SIGINT (Ctrl+C). Exiting...");
            g_run = 0;
            event_set(event_fd, SIGINT);
            workqueue_stop();
            break;
        case SIGTERM:
            LOG_WARN("[+] Received SIGTERM. Shutdown...");
            exit(0);
        case SIGABRT:
            LOG_WARN("[+] Received SIGABRT. Exiting...");
            event_set(event_fd, SIGABRT);
            break;
        default:
            LOG_WARN("[!] Received unidentified signal: %d", sig);
            break;
    }
}

int setup_signal_handler()
{
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        LOG_ERROR("Error registering signal SIGINT handler");
        return EINVAL;
    }

    if (signal(SIGTERM, sig_handler) == SIG_ERR) {
        LOG_ERROR("Error registering signal SIGTERM handler");
        return EINVAL;
    }

    if (signal(SIGABRT, sig_handler) == SIG_ERR) {
        LOG_ERROR("Error registering signal SIGABRT handler");
        return EINVAL;
    }

    return 0;
}

int main_loop()
{
    LOG_INFO("System manager service is running...");
    while (g_run) {
        usleep(200000);
    };

    LOG_INFO("System manager service is exiting...");
    return 0;
}

int main() {
    DBusConnection *conn;
    pthread_t dbus_listener;
    pthread_t task_handler;
    int ret = 0;

    if (setup_signal_handler()) {
        return EINVAL;
    }

    conn = setup_dbus();
    if (!conn) {
        LOG_FATAL("System Manager: Unable to establish connection with DBus");
        return -1;
    }

    ret = pthread_create(&task_handler, NULL, main_task_handler, NULL);
    if (ret) {
        LOG_FATAL("Failed to create worker thread: %s", strerror(ret));
        exit(1);
    }

    // Prepare eventfd to notify epoll when communicating with a thread
    ret = init_event_file();
    if (ret) {
        LOG_FATAL("Failed to initialize eventfd");
    }

    // This thread processes DBus messages for the system manager
    ret = pthread_create(&dbus_listener, NULL, dbus_listen_thread, conn);
    if (ret) {
        LOG_FATAL("Failed to create DBus listen thread: %s", strerror(ret));
        exit(1);
    }

    // System manager's primary tasks are executed within a loop
    ret = main_loop();
    if (ret) {
        event_set(event_fd, SIGUSR1);
        workqueue_stop();
    }

    pthread_join(dbus_listener, NULL);
    pthread_join(task_handler, NULL);

    close(event_fd);
    LOG_DEBUG("All services stopped. Safe exit.\n");
    return 0;
}
