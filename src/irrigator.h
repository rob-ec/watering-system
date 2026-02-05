/**
 * @file irrigator.h
 * @brief Definitions for Irrigator control.
 *
 * This file contains definitions for initialization and control of the Irrigator.
 * Obs.: "Irrigator" in the base projects stands for the rele that activates the solenoid.
 *
 * @author Robson Gomes
 * @email robson.mesquita56@gmail.com
 * @github github.com/rob-ec
 */

#ifndef IRRIGATOR_H
#define IRRIGATOR_H

#include "FreeRTOS.h"
#include "task.h"

// Irrigator pin acording the board adaptations described on this project documentation
#define IRRIGATOR_PIN 17

// Task notification values
#define IRRIGATOR_TURN_ON 1
#define IRRIGATOR_TURN_OFF 2
#define IRRIGATOR_REMOTE_TURN_ON 3
#define IRRIGATOR_REMOTE_TURN_OFF 4

#define IRRIGATOR_MAX_SCHEDULE_SIZE 4

struct schedule_item
{
    uint8_t hour;
    uint8_t minute;
    uint8_t duration; // in seconds
    uint8_t active;
};

typedef struct schedule_item schedule_item_t;

/**
 * @brief Handle for irrigator task.
 * Used to control the task (suspend/resume) from other tasks.
 */
extern TaskHandle_t irrigator_task_handle;

void irrigator_init(void);
void irrigator_turn_on(void);
void irrigator_turn_off(void);
void irrigator_toggle(void);
int irrigator_is_on(void);
void irrigator_set_schedule(int index, uint8_t hour, uint8_t minute, uint8_t duration, uint8_t active);
void irrigator_get_all_schedules(schedule_item_t *items);
void irrigator_task(void *pvParameters);

#endif // IRRIGATOR_H