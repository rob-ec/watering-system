/**
 * @file buzzer.h
 * @brief Definitions for buzzer control.
 *
 * This file contains definitions for initialization and control of the buzzer,
 * including the task that generates sound.
 *
 * @author Robson Gomes
 * @email robson.mesquita56@gmail.com
 * @github github.com/rob-ec
 */

#ifndef BUZZER_H
#define BUZZER_H

#include "FreeRTOS.h"
#include "task.h"

// Buzzer pin according to BitDogLab V6 schematic
#define BUZZER_PIN 21

#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_D5 587
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_G5 784
#define NOTE_A5 880
#define NOTE_B5 988

/**
 * @brief Handle for the buzzer task.
 * Used to control the task (suspend/resume) from other tasks.
 */
extern TaskHandle_t buzzer_task_handle;

/**
 * @brief Emits a beep with specific duration.
 * @param duration_ms Beep duration in milliseconds.
 */
void buzzer_beep(int duration_ms);

/**
 * @brief Plays a note with specific frequency and duration.
 * @param frequency Frequency in Hz.
 * @param duration_ms Duration in milliseconds.
 */
void buzzer_play_note(int frequency, int duration_ms);

/**
 * @brief Tries to play something close to "Song of Storms" from Zelda Ocarina of Time
 */
void buzzer_song_of_storms();

/**
 * @brief Task that controls the buzzer.
 * @param pvParameters Task parameters (unused).
 */
void buzzer_task(void *pvParameters);

#endif // BUZZER_H