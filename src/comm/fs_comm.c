/**
 * @file fs_comm.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys_comm.h>

#include <log.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  GLOBAL VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static int __sf_fs_write_internal(const char *path,  const char *data, \
                                  size_t len, int append)
{
    int fd, ret, flags;

    if (!path || !data || len == 0) {
        LOG_ERROR("invalid argument");
        return -1;
    }

    flags = O_WRONLY | O_CREAT;
    flags |= append ? O_APPEND : O_TRUNC;

    LOG_TRACE("%s to %s, len=%zu", \
         append ? "append" : "write", path, len);
    LOG_TRACE("write data:\n%.*s", (int)len, data);

    fd = open(path, flags, 0644);
    if (fd < 0) {
        LOG_ERROR("open failed: %s", strerror(errno));
        return -1;
    }

    ret = write(fd, data, len);
    if (ret < 0) {
        LOG_ERROR("write failed: %s", strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
    LOG_TRACE("%s success", append ? "append" : "write");
    return 0;
}

int gf_fs_write_file(const char *path, const char *data, size_t len)
{
    return __sf_fs_write_internal(path, data, len, 0);
}

int gf_fs_append_file(const char *path, const char *data, size_t len)
{
    return __sf_fs_write_internal(path, data, len, 1);
}

char *gf_fs_read_file(const char *path, size_t *out_len)
{
    int fd, ret;
    char *buf;
    off_t size;

    if (!path || !out_len) {
        LOG_ERROR("invalid argument");
        return NULL;
    }

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        LOG_ERROR("open failed: %s", strerror(errno));
        return NULL;
    }

    size = lseek(fd, 0, SEEK_END);
    if (size < 0) {
        LOG_ERROR("lseek failed: %s", strerror(errno));
        close(fd);
        return NULL;
    }
    lseek(fd, 0, SEEK_SET);

    buf = malloc(size + 1);
    if (!buf) {
        LOG_ERROR("malloc failed");
        close(fd);
        return NULL;
    }

    ret = read(fd, buf, size);
    if (ret < 0) {
        LOG_ERROR("read failed: %s", strerror(errno));
        free(buf);
        close(fd);
        return NULL;
    }

    buf[size] = '\0';
    *out_len = size;

    LOG_INFO("read from %s, len=%zu", path, size);
    LOG_DEBUG("content:\n%.*s", (int)size, buf);

    close(fd);
    return buf;
}

int gf_fs_file_exists(const char *path)
{
    struct stat st;

    if (!path) {
        LOG_ERROR("invalid argument");
        return 0;
    }

    if (stat(path, &st) == 0) {
        LOG_DEBUG("file exists: %s", path);
        return 1;
    }

    LOG_DEBUG("file not found: %s", path);
    return 0;
}
