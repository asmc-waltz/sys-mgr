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

int gf_fs_write_file(const char *path, const char *data, size_t len);
int gf_fs_append_file(const char *path, const char *data, size_t len);
char *gf_fs_read_file(const char *path, size_t *out_len);
int gf_fs_file_exists(const char *path);

#endif /* G_SYS_COMM_H */
