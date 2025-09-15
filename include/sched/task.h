/**
 * @file task.h
 *
 */

#ifndef G_TASK_H
#define G_TASK_H
/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <stdbool.h>

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
void normal_task_cnt_reset();
void normal_task_cnt_inc();
void normal_task_cnt_dec();
int32_t normal_task_cnt_get();
void endless_task_cnt_reset();
void endless_task_cnt_inc();
void endless_task_cnt_dec();
int32_t endless_task_cnt_get();
bool is_task_handler_idle();
void * main_task_handler(void* arg);

int32_t process_opcode_endless(uint32_t opcode, void *data);
int32_t process_opcode(uint32_t opcode, void *data);
int32_t create_local_simple_task(uint8_t flow, uint8_t duration, uint32_t opcode);
int32_t create_remote_task(uint8_t flow, void *data);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif /* G_TASK_H */
