/**
 * @file comm.h
 *
 */

#ifndef G_WORKQUEUE_H
#define G_WORKQUEUE_H
/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>

#include <comm/dbus_comm.h>

/*********************
 *      DEFINES
 *********************/
typedef enum {
    WORK_TYPE_LOCAL = 0,
    WORK_TYPE_REMOTE,
} work_type_t;

typedef enum {
    WORK_PRIO_LOW = 0,
    WORK_PRIO_NORMAL,
    WORK_PRIO_HIGH,
    WORK_PRIO_URGENT,
} work_priority_t;

typedef enum {
    WORK_DURATION_SHORT = 0,
    WORK_DURATION_LONG,
} work_duration_t;

/**********************
 *      TYPEDEFS
 **********************/
typedef struct work {
    work_type_t type;
    work_priority_t prio;
    work_duration_t duration;
    uint32_t opcode;
    void *data;
    struct work *next;
} work_t;

typedef struct workqueue {
    work_t *head;
    work_t *tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    atomic_int active_cnt;
} workqueue_t;

/**********************
 *  GLOBAL VARIABLES
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/*=====================
 * Setter functions
 *====================*/

/*=====================
 * Getter functions
 *====================*/

/*=====================
 * Other functions
 *====================*/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
work_t *create_work(uint8_t type, uint8_t priority, uint8_t duration, \
                    uint32_t opcode, void *data);
void delete_work(work_t *work);
void push_work(work_t *work);
work_t *pop_work_wait_safe();
void workqueue_handler_wakeup();
int32_t workqueue_active_count(void);

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif /* G_WORKQUEUE_H */
