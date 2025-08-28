/**
 * @file als.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
// #define LOG_LEVEL LOG_LEVEL_TRACE
#if defined(LOG_LEVEL)
#warning "LOG_LEVEL defined locally will override the global setting in this file"
#endif
#include <log.h>

#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <comm/f_comm.h>
#include <hw/common.h>

/*********************
 *      DEFINES
 *********************/
#define ALS_SAMPLE_TIME_CFG             "in_illuminance_integration_time"

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
int32_t als_late_init(const char *sensor_name)
{
    char device_path[MAX_PATH_LEN];
    char w_value[10];
    char f_path[128];
    int ret;

    if (!sensor_name) {
        printf("Invalid argument\n");
        return -EINVAL;
    }

    ret = find_device_path_by_name(IIO_DEV_SYSFS_PATH, IIO_DEV_NAME_FILE, \
                                   sensor_name, device_path, \
                                   sizeof(device_path));
    if (ret == 0) {
        LOG_INFO("Found IIO device: %s (target=%s)\n", device_path, sensor_name);
    } else if (ret == -ENOENT) {
        LOG_ERROR("Device '%s' not found\n", sensor_name);
        return ret;
    } else {
        LOG_ERROR("Error: %s\n", strerror(-ret));
        return ret;
    }

    snprintf(f_path, sizeof(f_path), "%s/%s", device_path, ALS_SAMPLE_TIME_CFG);
    if (gf_fs_file_exists(f_path)) {
        snprintf(w_value, sizeof(w_value), "%f", 0.1);
        ret = gf_fs_write_file(f_path, w_value, sizeof(w_value));
        if (ret) {
            LOG_ERROR("ALS sample configure failed, ret %d", ret);
        } else {

            LOG_INFO("ALS sample configured as %s, ret %d", w_value, ret);
        }
    }
}
