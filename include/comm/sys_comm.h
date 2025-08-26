/**
 * @file sys_comm.h
 *
 */

#ifndef G_SYS_COMM_H
#define G_SYS_COMM_H

/*********************
 *      INCLUDES
 *********************/
#include <stddef.h>
#include <stdint.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *  GLOBAL VARIABLES
 **********************/

/**********************
 *  GLOBAL PROTOTYPES
 **********************/
// internal comm
int32_t event_set(int32_t evfd, uint64_t code);
int32_t event_get(int32_t evfd, uint64_t *out_val);
int32_t init_event_file();
int32_t cleanup_event_file(void);

// fs_comm
int32_t gf_fs_write_file(const char *path, const char *data, size_t len);
int32_t gf_fs_append_file(const char *path, const char *data, size_t len);
char *gf_fs_read_file(const char *path, size_t *out_len);
int32_t gf_fs_file_exists(const char *path);

// proc_comm
int32_t exec_cmd_with_interact(char *cmd, char *input);


#endif /* G_SYS_COMM_H */
