/**
 * @file f_comm.h
 *
 */

#ifndef G_F_COMM_H
#define G_F_COMM_H

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
int gf_fs_read_file(const char *path, char *buf, size_t buf_len, \
                    size_t *out_len);
int32_t gf_fs_file_exists(const char *path);

int32_t find_device_path_by_name(const char *basepath, const char *fid, \
                                 const char *id_str, char *result_path, \
                                 size_t result_max_len);
// proc_comm
int32_t exec_cmd_with_interact(char *cmd, char *input);


#endif /* G_F_COMM_H */
