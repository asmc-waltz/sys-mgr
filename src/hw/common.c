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
int32_t common_hardware_init()
{
    backlight_setup();
    brightness_ramp(0, 100, 500000);

    char dev_path[MAX_PATH_LEN];
    als_late_init("opt3001", dev_path, sizeof(dev_path));
    als_read_illuminance(dev_path);
}

int32_t common_hardware_deinit()
{
    backlight_setup();
    brightness_ramp(100, 0, 500000);
}

int32_t hardware_monitor_loop()
{
    ;
}
