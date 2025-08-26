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
int32_t set_brightness(uint8_t bl_peretent);

int32_t rumble_trigger(uint32_t event_id, uint32_t ff_type, uint32_t duration);

/*=====================
 * Getter functions
 *====================*/

/*=====================
 * Other functions
 *====================*/
int32_t backlight_setup();

/**********************
 *      MACROS
 **********************/

#endif /* G_COMMON_H */
