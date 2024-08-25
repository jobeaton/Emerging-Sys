/*
 * Copyright (c) 2015-2020, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== gpiointerrupt.c ========
 */
#include <stdint.h>
#include <stddef.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>

/* Driver configuration */
#include "ti_drivers_config.h"
#include <ti/drivers/Timer.h>

#define DOT_DURATION      500000   // dot
#define DASH_DURATION     1500000  // dash
#define CHAR_SPACE        1500000  // character space
#define WORD_SPACE        3500000  // word space

typedef enum {
    STATE_SOS, //Sos
    STATE_OK, //OK
    STATE_IDLE //Idle
} MessageState;

typedef enum {
    LED_RED,   // dot
    LED_GREEN, // dash
    LED_OFF    // space
} LEDState;

MessageState currentState = STATE_SOS;
uint8_t morseIndex = 0; //Index for node
uint32_t blinkTiming = 0;
uint32_t delayCounter = 0;
const uint8_t morseCodeSOS[] = { 1, 1, 1, 0, 2, 2, 2, 0, 1, 1, 1 }; // SOS
const uint8_t morseCodeOK[] = { 2, 1, 2, 0, 1 }; // OK
uint8_t buttonPressed = 0;
LEDState LED_STATE = LED_OFF;
// sets the states of the LEDs
void setMorseLEDs() {
    switch(LED_STATE) {
        case LED_RED:
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
            GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
            printf("RED\n"); // Debug print
            break;
        case LED_GREEN:
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
            GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_ON);
            printf("GREEN\n"); // Debug print
            break;
        case LED_OFF:
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
            GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
            printf("OFF\n"); // Debug print
            break;
        default:
            break;
    }
}

void timerCallback(Timer_Handle myHandle, int_fast16_t status)
{

    // Check if delay
    if (delayCounter < blinkTiming) {
        delayCounter++;
        return;
    }

    // Resets the counter
    delayCounter = 0;

    if (currentState == STATE_SOS) {
        if (morseIndex < sizeof(morseCodeSOS) / sizeof(morseCodeSOS[0])) {
            printf("Processing SOS, Morse Index: %d\n", morseIndex);
            if (morseCodeSOS[morseIndex] == 1) { // Dot
                LED_STATE = LED_RED;
                blinkTiming = DOT_DURATION; // time for dot
            } else if (morseCodeSOS[morseIndex] == 2) { // Dash
                LED_STATE = LED_GREEN;
                blinkTiming = DASH_DURATION; // time for dash
            } else { // Space
                LED_STATE = LED_OFF;
                blinkTiming = CHAR_SPACE; // time for space
                morseIndex++; // next symbol
            }
            // Update LEDs before moving to the next symbol
            setMorseLEDs(); // Set LED states for the current LED_STATE
            morseIndex++;
        } else {
            // reset
            morseIndex = 0;
            LED_STATE = LED_OFF; // lights off
            setMorseLEDs(); // off
            blinkTiming = WORD_SPACE; // timefor word space
        }
    } else if (currentState == STATE_OK) {
        if (morseIndex < sizeof(morseCodeOK) / sizeof(morseCodeOK[0])) {
            printf("Processing OK, Morse Index: %d\n", morseIndex);
            if (morseCodeOK[morseIndex] == 1) { // Dot
                LED_STATE = LED_RED;
                blinkTiming = DOT_DURATION; // time for dot
            } else if (morseCodeOK[morseIndex] == 2) { // Dash
                LED_STATE = LED_GREEN;
                blinkTiming = DASH_DURATION; // time for dash
            } else { // Space
                LED_STATE = LED_OFF;
                blinkTiming = CHAR_SPACE; // time for space
                morseIndex++; // next symbol
            }
            setMorseLEDs();
            morseIndex++;
        } else {
            // reset
            morseIndex = 0;
            LED_STATE = LED_OFF;
            setMorseLEDs(); // Turn off
            blinkTiming = WORD_SPACE; // time for word space
        }
    }
}
void initTimer(void)
{
Timer_Handle timer0;
Timer_Params params;
Timer_init();
Timer_Params_init(&params);
params.period = 500000;
params.periodUnits = Timer_PERIOD_US;
params.timerMode = Timer_CONTINUOUS_CALLBACK;
 params.timerCallback = timerCallback;

timer0 = Timer_open(CONFIG_TIMER_0, &params);
if (timer0 == NULL) {
/* Failed to initialized timer */
while (1) {}
}
if (Timer_start(timer0) == Timer_STATUS_ERROR) {
/* Failed to start timer */
while (1) {}
}
}
/*
 *  ======== gpioButtonFxn0 ========
 *  Callback function for the GPIO interrupt on CONFIG_GPIO_BUTTON_0.
 *
 *  Note: GPIO interrupts are cleared prior to invoking callbacks.
 */
void gpioButtonFxn0(uint_least8_t index)
{
    if (delayCounter >= blinkTiming) {
        // Toggle the message state
        if (currentState == STATE_SOS) {
            currentState = STATE_OK;
        } else {
            currentState = STATE_SOS;
        }
        buttonPressed = 1; // Set flag
    }
}

/*
 *  ======== gpioButtonFxn1 ========
 *  Callback function for the GPIO interrupt on CONFIG_GPIO_BUTTON_1.
 *  This may not be used for all boards.
 *
 *  Note: GPIO interrupts are cleared prior to invoking callbacks.
 */
void gpioButtonFxn1(uint_least8_t index)
{
    /* Toggle an LED */
    GPIO_toggle(CONFIG_GPIO_LED_1);
}

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    /* Call driver init functions */
    GPIO_init();

    /* Configure the LED and button pins */
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_LED_1, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_BUTTON_0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

    /* Turn on user LED */
    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);

    /* Install Button callback */
    GPIO_setCallback(CONFIG_GPIO_BUTTON_0, gpioButtonFxn0);

    /* Enable interrupts */
    GPIO_enableInt(CONFIG_GPIO_BUTTON_0);


    if (CONFIG_GPIO_BUTTON_0 != CONFIG_GPIO_BUTTON_1)
    {

        GPIO_setConfig(CONFIG_GPIO_BUTTON_1, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

        GPIO_setCallback(CONFIG_GPIO_BUTTON_1, gpioButtonFxn1);
        GPIO_enableInt(CONFIG_GPIO_BUTTON_1);
    }

    initTimer(); // Initialize the timer

    return (NULL);
}
