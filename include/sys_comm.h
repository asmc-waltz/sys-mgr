/**
 * @file sys_comm.h
 *
 */

#ifndef G_COMM_H
#define G_COMM_H

#include <stdint.h>
#include <dbus/dbus.h>

#define SYS_MGR_DBUS_SER                "com.SystemManager.Service"
#define SYS_MGR_DBUS_OBJ_PATH           "/com/SystemManager/Obj/SysCmd"
#define SYS_MGR_DBUS_IFACE              "com.SystemManager.Interface"
#define SYS_MGR_DBUS_METH               "SysMeth"

#define UI_DBUS_SER                     "com.TerminalUI.Service"
#define UI_DBUS_OBJ_PATH                "/com/TerminalUI/Obj/UsrCmd"
#define UI_DBUS_IFACE                   "com.TerminalUI.Interface"
#define UI_DBUS_SIG                     "UISig"

int init_event_file();
void event_set(int evfd, uint64_t code);
uint64_t event_get(int evfd);

DBusConnection * setup_dbus();
void* dbus_listen_thread(void* arg);

#endif /* G_COMM_H */
