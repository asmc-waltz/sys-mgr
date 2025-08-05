/**
 * @file dbus_comm.h
 *
 */

#ifndef G_SYS_COMM_H
#define G_SYS_COMM_H

#include <sys_comm/network.h>

int init_event_file();
void event_set(int evfd, uint64_t code);
uint64_t event_get(int evfd);

#endif /* G_SYS_COMM_H */
