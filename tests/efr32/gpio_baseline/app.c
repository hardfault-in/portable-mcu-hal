/***************************************************************************//**
 * @file
 * @brief Top level application functions
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
#include "blink.h"

// Verification-pipeline log line (build id/vendor/test come from scripts/build.sh).
// Printed over SEGGER RTT via the SDK's iostream retarget_stdio.
#define LOG_BOOT() printf("[BOOT] build=%s vendor=%s test=%s\r\n", BUILD_ID, BUILD_VENDOR, BUILD_TEST)

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  LOG_BOOT();
  blink_init();
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  blink_process_action();
}
