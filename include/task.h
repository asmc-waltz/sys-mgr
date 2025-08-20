/**
 * @file task.h
 *
 */

#ifndef G_TASK_H
#define G_TASK_H
/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>

#include <log.h>

/*********************
 *      DEFINES
 *********************/
#define MAX_ENTRIES 32

/**********************
 *      TYPEDEFS
 **********************/
/* Go001 DBus message frame
    STRUCT {
        STRING   component_id;
        INT32    topic_id;
        INT32    opcode;

        ARRAY of STRUCT {
            STRING key;
            INT32  data_type;
            INT32  data_length;
            VARIANT data;
        }
    }
 */

typedef enum {
    COMP_ID_NONE = 0,
    COMP_ID_SYS_MGR,
    COMP_ID_TERM_UI,
} component_id_t;

typedef enum {
    TPC_ID_NONE = 0,
    OP_ID_NET,
} topic_id_t;

typedef enum {
    OP_ID_NONE = 0,
    /* Common API */
    OP_ID_START_DBUS,
    OP_ID_PING,
    /* Network API */
    OP_ID_WIFI_RESCAN,
    OP_ID_WIFI_GET_AP_LIST,
    OP_ID_WIFI_GET_AP_INFO,
    OP_ID_WIFI_CONN_AP,
    /* Vibrator API */
    OP_ID_LEFT_VIBRATOR,
    OP_ID_RIGHT_VIBRATOR,
    /* IMU API */
    OP_ID_START_IMU,
    OP_ID_STOP_IMU,
    OP_ID_READ_IMU,
    /* AUDIO API */
    OP_ID_AUDIO_INIT,
    OP_ID_AUDIO_RELEASE,
    OP_ID_SOUND_PLAY,
} opcode_t;


// Union for holding the actual variant value
typedef union {
    const char *str;     // String value
    int32_t i32;         // Signed 32-bit integer
    uint32_t u32;        // Unsigned 32-bit integer
    double dbl;          // Double-precision float
} variant_val_t;

// One entry in the payload array
typedef struct {
    const char *key;         // Entry key name
    uint32_t data_type;       // DBus data type code (e.g., DBUS_TYPE_STRING)
    uint32_t data_length;     // Data length (used if type is array/string)
    variant_val_t value;      // Actual value
} payload_t;

// Top-level data frame structure
typedef struct {
    const char *component_id;    // Identifier of the sender
    uint32_t topic_id;            // Topic ID
    uint32_t opcode;              // Operation code
    uint32_t entry_count;         // Number of entries in the payload
    payload_t entries[MAX_ENTRIES]; // Payload entries
} remote_cmd_t;




typedef struct {
    uint32_t opcode;
    void *payload;
} local_cmd_t;

/**********************
 *  GLOBAL VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  GLOBAL PROTOTYPES
 **********************/
void normal_task_cnt_reset();
void normal_task_cnt_inc();
void normal_task_cnt_dec();
int32_t normal_task_cnt_get();
void endless_task_cnt_reset();
void endless_task_cnt_inc();
void endless_task_cnt_dec();
int32_t endless_task_cnt_get();

int process_opcode(uint32_t opcode, void *data);
int create_local_simple_task(uint8_t flow, uint8_t duration, uint32_t opcode);

bool is_task_handler_idle();
void * main_task_handler(void* arg);
/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif /* G_TASK_H */
