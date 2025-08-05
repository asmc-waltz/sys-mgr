/**
 * @file dbus_comm.h
 *
 */

#ifndef G_DBUS_COMM_H
#define G_DBUS_COMM_H

#include <stdint.h>
#include <stdbool.h>
#include <dbus/dbus.h>

#include <log.h>

#define SYS_MGR_DBUS_SER                "com.SystemManager.Service"
#define SYS_MGR_DBUS_OBJ_PATH           "/com/SystemManager/Obj/SysCmd"
#define SYS_MGR_DBUS_IFACE              "com.SystemManager.Interface"
#define SYS_MGR_DBUS_METH               "SysMeth"

#define UI_DBUS_SER                     "com.TerminalUI.Service"
#define UI_DBUS_OBJ_PATH                "/com/TerminalUI/Obj/UsrCmd"
#define UI_DBUS_IFACE                   "com.TerminalUI.Interface"
#define UI_DBUS_SIG                     "UISig"

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
    OP_ID_PING,
    /* Network API */
    OP_ID_WIFI_RESCAN,
    OP_ID_WIFI_GET_AP_LIST,
    OP_ID_WIFI_GET_AP_INFO,
    OP_ID_WIFI_CONN_AP,
} opcode_t;

#define MAX_ENTRIES 32

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
} cmd_data_t;

// Encode cmd_data_t into DBusMessage
bool encode_data_frame(DBusMessage *msg, const cmd_data_t *frame);

// Decode DBusMessage into cmd_data_t
bool decode_data_frame(DBusMessage *msg, cmd_data_t *out);

DBusConnection * setup_dbus();
void* dbus_listen_thread(void* arg);

#endif /* G_DBUS_COMM_H */
