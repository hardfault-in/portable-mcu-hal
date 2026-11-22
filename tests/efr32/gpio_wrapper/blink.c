/***************************************************************************//**
 * @file
 * @brief Blink examples functions
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#include <stdio.h>
#include "sl_simple_led.h"
#include "sl_simple_led_instances.h"
#include "sl_sleeptimer.h"
#include "sl_code_classification.h"
#include "sl_gpio.h"
#include "sl_simple_led_led0_config.h"
#include "hal_wrapper.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

// WRAPPER CHANGE: the LED is driven through hf_gpio_* on the same pin the
// SDK's led0 instance uses, instead of through sl_led_*.
#define LED_PIN         HF_PIN(SL_SIMPLE_LED_LED0_PORT, SL_SIMPLE_LED_LED0_PIN)
#define LED_ACTIVE_HIGH (SL_SIMPLE_LED_LED0_POLARITY == SL_SIMPLE_LED_POLARITY_ACTIVE_HIGH)

#ifndef TOOGLE_DELAY_MS
#define TOOGLE_DELAY_MS         500
#endif

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/

#define LOG_TOGGLE_COUNT        6

sl_sleeptimer_timer_handle_t timer;
volatile bool toggle_timeout = false;
static int toggle_count = 0;

/*******************************************************************************
 *********************   LOCAL FUNCTION PROTOTYPES   ***************************
 ******************************************************************************/

SL_CODE_RAM static void on_timeout(sl_sleeptimer_timer_handle_t *handle,
                                   void *data);

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * Initialize blink example.
 ******************************************************************************/
void blink_init(void)
{
  // WRAPPER CHANGE: configure the pin through the wrapper and start with the
  // LED off, as the SDK's led driver does.
  hf_gpio_init(LED_PIN, HF_GPIO_OUTPUT_PUSHPULL);
  hf_gpio_write(LED_PIN, !LED_ACTIVE_HIGH);

  // Create timer for waking up the system periodically.
  sl_sleeptimer_start_periodic_timer_ms(&timer,
                                        TOOGLE_DELAY_MS,
                                        on_timeout, NULL,
                                        0,
                                        SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
}

/***************************************************************************//**
 * Blink ticking function.
 ******************************************************************************/
void blink_process_action(void)
{
  if (toggle_timeout == true) {
    hf_gpio_toggle(LED_PIN);   // WRAPPER CHANGE (was sl_led_toggle)
    toggle_timeout = false;

    // Verification-pipeline log lines: one per toggle for the first
    // LOG_TOGGLE_COUNT toggles, then [DONE]; the LED keeps blinking.
    if (toggle_count < LOG_TOGGLE_COUNT) {
      toggle_count++;
      printf("[GPIO] toggle led=0 count=%d state=%d\r\n",
             toggle_count, (int)(hf_gpio_read(LED_PIN) == LED_ACTIVE_HIGH));   // WRAPPER CHANGE (was sl_led_get_state)
      if (toggle_count == LOG_TOGGLE_COUNT) {
        printf("[DONE] test=%s\r\n", BUILD_TEST);
      }
    }
  }
}

/***************************************************************************//**
 * Sleeptimer timeout callback.
 ******************************************************************************/
static void on_timeout(sl_sleeptimer_timer_handle_t *handle,
                       void *data)
{
  (void)&handle;
  (void)&data;
  toggle_timeout = true;
}
