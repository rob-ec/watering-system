/**
 * @file button.h
 * @brief Definitions for button control.
 *
 * This file contains pin definitions, the button task,
 * and debounce time.
 *
 * @author Robson Gomes
 * @email robson.mesquita56@gmail.com
 * @github github.com/rob-ec
 */

#ifndef BUTTON_H
#define BUTTON_H

// Button pins according to BitDogLab V6 schematic
#define BUTTON_A_PIN 5
#define BUTTON_B_PIN 6

// Debounce time in milliseconds to avoid multiple readings
#define DEBOUNCE_TIME_MS 200

/**
 * @brief Task that monitors the buttons.
 * @param pvParameters Task parameters (unused).
 */
void button_task(void *pvParameters);

#endif // BUTTON_H