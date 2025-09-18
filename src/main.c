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
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <sys/epoll.h>
#include <dbus/dbus.h>

#include "comm/dbus_comm.h"
#include "comm/f_comm.h"
#include "comm/net/network.h"
#include "comm/cmd_payload.h"
#include "sched/workqueue.h"

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
static void service_shutdown_flow();

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
            service_shutdown_flow();
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
        return -EIO;
    }

    if (signal(SIGTERM, sig_handler) == SIG_ERR) {
        LOG_ERROR("Error registering signal SIGTERM handler");
        return -EIO;
    }

    if (signal(SIGABRT, sig_handler) == SIG_ERR) {
        LOG_ERROR("Error registering signal SIGABRT handler");
        return -EIO;
    }

    return 0;
}

static int32_t service_startup_flow(void)
{
    int32_t ret;
    pthread_t dbus_handler;

    /* Prepare eventfd to notify epoll when communicating with threads */
    ret = init_event_file();
    if (ret) {
        LOG_FATAL("Failed to initialize eventfd, ret=%d", ret);
        goto exit_err;
    }

    ret = workqueue_init();
    if (ret) {
        LOG_FATAL("Failed to initialize workqueues, ret=%d", ret);
        goto exit_event;
    }

    /* Create DBus listener thread */
    ret = pthread_create(&dbus_handler, NULL, dbus_fn_thread_handler, NULL);
    if (ret) {
        LOG_FATAL("Failed to create DBus listener thread: %s", strerror(ret));
        goto exit_workqueue;
    }


    ret = network_manager_comm_init();
    if (ret) {
        LOG_FATAL("Failed to create network manager client: %s", strerror(ret));
        goto exit_dbus;
    }


    create_local_simple_task(WORK_PRIO_NORMAL, WORK_DURATION_SHORT, OP_AUDIO_INIT);

    LOG_INFO("System Manager initialization completed");
    return 0;

/* Cleanup sequence in case of failure */
exit_dbus:
    event_set(event_fd, SIGUSR1);

exit_workqueue:
    workqueue_deinit();

exit_event:
    cleanup_event_file();

exit_err:
    return ret;
}

/**
 * Gracefully shutdown the system services
 *
 * This function ensures all normal tasks are finished, then stops
 * endless tasks and notifies system and DBus about shutdown.
 */
static void service_shutdown_flow(void)
{
    int32_t ret;
    int32_t cnt;


    create_local_simple_task(WORK_PRIO_NORMAL, WORK_DURATION_SHORT, OP_AUDIO_RELEASE);

    /* Wait until workqueue is fully drained */
    cnt = workqueue_active_count(get_wq(SYSTEM_WQ));
    while (cnt) {
        LOG_TRACE("Waiting for workqueue to be free, remaining work %d", cnt);
        usleep(100000);
        cnt = workqueue_active_count(get_wq(SYSTEM_WQ));
    }

    /* Stop background threads and notify shutdown */
    g_run = 0;                      /* Signal threads to stop */

    event_set(event_fd, SIGINT);    /* Notify DBus/system about shutdown */

    workqueue_deinit();

    cleanup_event_file();

    LOG_INFO("Service shutdown flow completed");
}


static int32_t main_loop()
{
    LOG_INFO("System manager service is running...");
    while (g_run) {
        usleep(200000);
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
    ret = setup_signal_handler();
    if (ret) {
        return ret;
    }

    ret = service_startup_flow();
    if (ret) {
        LOG_FATAL("System Manager init flow failed, ret=%d", ret);
        return ret;
    }

    ret = main_loop();
    if (ret) {
        return ret;
    }

    LOG_INFO("|-------------> All services stopped. Safe exit <-------------|");
    return 0;
}
