/**
 * @file backlight.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>

#include <comm/sys_comm.h>

#include <log.h>

/*********************
 *      DEFINES
 *********************/
#define FS_BRIGHTNESS               "/sys/class/backlight/backlight/brightness"
#define FS_BRIGHTNESS_POWER         "/sys/class/backlight/backlight/bl_power"

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
int32_t backlight_setup()
{
    int32_t rc;
    char str[10];

    if (gf_fs_file_exists(FS_BRIGHTNESS_POWER)) {
        sprintf(str, "%d", 0);
        rc = gf_fs_write_file(FS_BRIGHTNESS_POWER, str, sizeof(str));
    }

    return rc;
}

int32_t set_brightness(uint8_t bl_percent)
{
    char str_percent[10];

    // TODO: workaround when backlight cannot be set to zero from kernel
    if (bl_percent == 0) {
        bl_percent = 1;
    }

    sprintf(str_percent, "%d", bl_percent);
    gf_fs_write_file(FS_BRIGHTNESS, str_percent, sizeof(str_percent));
}

