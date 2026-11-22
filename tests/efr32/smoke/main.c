/* portable-mcu-hal hardware verification: EFR32 "smoke" test.
 *
 * Hand-written for this verification pipeline, NOT copied from a Silicon
 * Labs example, so it carries no vendor license header. The build
 * (scripts/build.sh) generates a BRD2601B project from the SDK's own
 * blink_baremetal example and overlays this file as its main.c, keeping
 * the generated board init / clock / pin configuration.
 *
 * Logging goes through the SDK's native iostream/retarget_stdio over the
 * board VCOM (115200 8N1), never through the wrapper's own UART adapter,
 * so a wrapper peripheral bug can't hide behind a logging bug.
 *
 * The LED is driven through the wrapper's hf_gpio_* API, not the SDK's
 * sl_simple_led driver. LED0 is port D pin 2 on BRD2601B, taken from the
 * generated sl_simple_led_led0_config.h (SL_SIMPLE_LED_LED0_PORT/_PIN).
 */
#include <stdbool.h>
#include <stdio.h>

#include "sl_gpio.h"
#include "sl_main_init.h"
#include "sl_simple_led_led0_config.h"

#include "hal_wrapper.h"

#if !defined(BUILD_ID) || !defined(BUILD_VENDOR) || !defined(BUILD_TEST)
#error "BUILD_ID, BUILD_VENDOR and BUILD_TEST must be defined by scripts/build.sh"
#endif

#define SMOKE_LED_PIN HF_PIN(SL_SIMPLE_LED_LED0_PORT, SL_SIMPLE_LED_LED0_PIN)
#define SMOKE_TOGGLE_COUNT 5

int main(void)
{
    sl_main_init();

    printf("[BOOT] build=%s vendor=%s test=%s\r\n", BUILD_ID, BUILD_VENDOR, BUILD_TEST);

    bool ok = hf_gpio_init(SMOKE_LED_PIN, HF_GPIO_OUTPUT_PUSHPULL);
    printf("[GPIO] init pin=0x%02x ret=%s\r\n", (unsigned)SMOKE_LED_PIN, ok ? "OK" : "ERR");

    for (int i = 0; i < SMOKE_TOGGLE_COUNT; i++) {
        hf_gpio_toggle(SMOKE_LED_PIN);
        bool state = hf_gpio_read(SMOKE_LED_PIN);
        printf("[GPIO] toggle pin=0x%02x val=%d\r\n", (unsigned)SMOKE_LED_PIN, state ? 1 : 0);
    }

    printf("[APP] toggles=%d\r\n", SMOKE_TOGGLE_COUNT);
    printf("[DONE] test=%s\r\n", BUILD_TEST);

    while (1) {
        /* idle: log capture stops at [DONE] */
    }
}
