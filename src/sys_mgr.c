#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/epoll.h>
#include <dbus/dbus.h>

#include <sys_mgr.h>
#include <sys_comm.h>
#include <workqueue.h>

extern int event_fd;
volatile sig_atomic_t g_run = 1;

void sig_handler(int sig) {
    switch (sig) {
        case SIGINT:
            printf("\n[+] Received SIGINT (Ctrl+C). Exiting...\n");
            g_run = 0;
            event_set(event_fd, SIGINT);
            workqueue_stop();
            break;
        case SIGTERM:
            printf("\n[+] Received SIGTERM. Shutdown...\n");
            exit(0);
        case SIGABRT:
            printf("\n[+] Received SIGABRT. Exiting...\n");
            event_set(event_fd, SIGABRT);
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
        return EINVAL;
    }

    if (signal(SIGTERM, sig_handler) == SIG_ERR) {
        perror("Error registering signal SIGTERM handler");
        return EINVAL;
    }

    if (signal(SIGABRT, sig_handler) == SIG_ERR) {
        perror("Error registering signal SIGABRT handler");
        return EINVAL;
    }

    return 0;
}

int main_loop()
{
    printf("System manager service is running...\n");
    while (g_run) {
        usleep(200000);
    };

    printf("System manager service is exiting...\n");
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
        printf("System Manager: Unable to establish connection with DBus\n");
        return -1;
    }

    ret = pthread_create(&task_handler, NULL, main_task_handler, NULL);
    if (ret) {
        printf("Failed to create worker thread: %s\n", strerror(ret));
        exit(1);
    }

    // Prepare eventfd to notify epoll when communicating with a thread
    init_event_file();

    // This thread processes DBus messages for the system manager
    ret = pthread_create(&dbus_listener, NULL, dbus_listen_thread, conn);
    if (ret) {
        printf("Failed to create DBus listen thread: %s\n", strerror(ret));
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
    printf("All services stopped. Safe exit.\n");
    return 0;
}
