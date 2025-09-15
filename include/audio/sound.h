/**
 * @file sound.h
 *
 */

#ifndef G_SOUND_H
#define G_SOUND_H
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
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  GLOBAL PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
int32_t snd_sys_init();
void snd_sys_release();
int32_t audio_play_sound(const char *snd_file);

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif /* G_SOUND_H */
