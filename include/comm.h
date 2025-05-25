/**
 * @file comm.h
 *
 */

#ifndef G_COMM_H
#define G_COMM_H

#include <stdint.h>
#include <dbus/dbus.h>

#define SERVICE_NAME "com.SystemManager.Service"
#define OBJECT_PATH "/com/SystemManager/Obj/SysCmd"
#define INTERFACE_NAME "com.SystemManager.Interface"

int init_event_file();
void event_set(int evfd, uint64_t code);
uint64_t event_get(int evfd);

DBusConnection * setup_dbus();
void* dbus_listen_thread(void* arg);

#endif /* G_COMM_H */
