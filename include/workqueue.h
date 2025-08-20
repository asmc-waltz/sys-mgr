/**
 * @file workqueue.h
 *
 */

#ifndef G_WORKQUEUE_H
#define G_WORKQUEUE_H
/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <pthread.h>

#include <dbus_comm.h>

/*********************
 *      DEFINES
 *********************/
typedef enum {
    LOCAL = 0,
    REMOTE,
} work_type_t;

typedef enum {
    BLOCK = 0,
    NON_BLOCK,
} work_flow_t;

typedef enum {
    SHORT = 0,
    LONG,
    ENDLESS,
} work_duration_t;

/**********************
 *      TYPEDEFS
 **********************/
typedef struct work {
    uint8_t type;
    uint8_t flow;
    uint8_t duration;
    uint32_t opcode;
    void *data;
    struct work *next;
} work_t;

typedef struct workqueue {
    work_t *head;
    work_t *tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
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
work_t *create_work(uint8_t type, uint8_t flow, uint8_t duration, \
                    uint32_t opcode, void *data);
void delete_work(work_t *work);
void push_work(work_t *work);
work_t* pop_work_wait();
void workqueue_stop();

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif /* G_WORKQUEUE_H */
