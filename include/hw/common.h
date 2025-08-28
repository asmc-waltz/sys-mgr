/**
 * @file common.h
 *
 */
#ifndef G_COMMON_H
#define G_COMMON_H

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/
#define MAX_PATH_LEN                    256
#define IIO_DEV_SYSFS_PATH              "/sys/bus/iio/devices"
#define IIO_DEV_NAME_FILE               "name"

#define ALS_SENSOR_NAME                 "opt3001"
/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  GLOBAL VARIABLES
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/*=====================
 * Setter functions
 *====================*/
int32_t backlight_setup();
int32_t set_brightness(uint8_t bl_peretent);
int32_t brightness_ramp(uint8_t from, uint8_t to, uint32_t period_us);
int32_t rumble_trigger(uint32_t event_id, uint32_t ff_type, uint32_t duration);

/*=====================
 * Getter functions
 *====================*/

/*=====================
 * Other functions
 *====================*/
int32_t als_late_init(const char *sensor_name);

int32_t hw_init();
int32_t hw_deinit();
/**********************
 *      MACROS
 **********************/

#endif /* G_COMMON_H */
