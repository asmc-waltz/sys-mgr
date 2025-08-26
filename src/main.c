/**
 * @file main.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
// #define LOG_LEVEL LOG_LEVEL_TRACE
#if defined(LOG_LEVEL)
#warning "LOG_LEVEL defined locally will override the global setting in this file"
#endif
#include <log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <sys/epoll.h>
#include <dbus/dbus.h>

#include <comm/dbus_comm.h>
#include <comm/sys_comm.h>
#include <comm/net/network.h>
#include <sched/workqueue.h>
#include <sched/task.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  GLOBAL VARIABLES
 **********************/
extern int32_t event_fd;
volatile sig_atomic_t g_run = 1;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void sig_handler(int32_t sig)
{
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

static int32_t setup_signal_handler()
{
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        LOG_ERROR("Error registering signal SIGINT handler");
        return -1;
    }

    if (signal(SIGTERM, sig_handler) == SIG_ERR) {
        LOG_ERROR("Error registering signal SIGTERM handler");
        return -1;
    }

    if (signal(SIGABRT, sig_handler) == SIG_ERR) {
        LOG_ERROR("Error registering signal SIGABRT handler");
        return -1;
    }

    return 0;
}


static int32_t main_loop()
{
    uint32_t cnt = 0;

    LOG_INFO("System manager service is running...");
    while (g_run) {
        usleep(200000);
        if (++cnt == 20) {
            cnt = 0;
            is_task_handler_idle();
        }
    };

    LOG_INFO("System manager service is exiting...");
    return 0;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
int32_t main(void)
{
    pthread_t task_handler;
    int32_t ret = 0;

    LOG_INFO("|---------------------> SYSTEM MANAGER <----------------------|");
    if (setup_signal_handler()) {
        goto exit_error;
    }

    ret = pthread_create(&task_handler, NULL, main_task_handler, NULL);
    if (ret) {
        LOG_FATAL("Failed to create worker thread: %s", strerror(ret));
        goto exit_error;
    }

    // Prepare eventfd to notify epoll when communicating with a thread
    ret = init_event_file();
    if (ret) {
        LOG_FATAL("Failed to initialize eventfd");
        goto exit_workqueue;
    }

    create_local_simple_task(NON_BLOCK, ENDLESS, OP_START_DBUS);
    create_local_simple_task(NON_BLOCK, SHORT, OP_AUDIO_INIT);

    ret = network_manager_comm_init();
    if (ret) {
        LOG_FATAL("Failed to create network manager client: %s", strerror(ret));
        goto exit_listener;
    }

    // System manager's primary tasks are executed within a loop
    ret = main_loop();
    if (ret) {
        goto exit_nm;
    }

    pthread_join(task_handler, NULL);
    // TODO: release audio HW
    snd_sys_release();
    cleanup_event_file();

    LOG_INFO("|-------------> All services stopped. Safe exit <-------------|");
    return 0;

exit_nm:
    network_manager_comm_deinit();

exit_listener:
    event_set(event_fd, SIGUSR1);

exit_workqueue:
    workqueue_stop();
    pthread_join(task_handler, NULL);
    cleanup_event_file();

exit_error:
    return -1;
}
