/**
 * @file common.c
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <comm/f_comm.h>
#include <hw/common.h>

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
void test_find_iio_device(const char *find_id)
{
    char device_path[MAX_PATH_LEN];
    int ret;

    if (!find_id) {
        printf("Invalid argument\n");
        return;
    }

    ret = find_device_path_by_name(IIO_DEV_SYSFS_PATH, IIO_DEV_NAME_FILE, \
                                   find_id, device_path, \
                                   sizeof(device_path));
    if (ret == 0) {
        printf("Found IIO device: %s (target=%s)\n", device_path, find_id);
    } else if (ret == -ENOENT) {
        printf("Device '%s' not found\n", find_id);
    } else {
        printf("Error: %s\n", strerror(-ret));
    }
}

int32_t hw_init()
{
    backlight_setup();
    brightness_ramp(0, 100, 500000);
    test_find_iio_device("opt3001");
    test_find_iio_device("mpu6500");
    test_find_iio_device("bmp280");
}

int32_t hw_deinit()
{
    backlight_setup();
    brightness_ramp(100, 0, 500000);
}

int32_t hw_monitor_loop()
{
    ;
}
