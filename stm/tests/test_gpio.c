/* Unit tests for stm/gpio/hal_port_stm32.c against tests/fakes/stm32. */
#include "unity.h"
#include "hal_wrapper.h"
#include "fake_stm32_hal.h"

void setUp(void) { fake_stm32_reset(); }
void tearDown(void) {}

void test_init_valid_pin_enables_clock_and_returns_true(void)
{
    hf_gpio_t pin = HF_PIN(0, 3); /* port A, pin 3 */
    TEST_ASSERT_TRUE(hf_gpio_init(pin, HF_GPIO_OUTPUT_PUSHPULL));
    TEST_ASSERT_EQUAL_INT(1, fake_stm32_rcc_enable_count[0]);
}

void test_init_port_beyond_table_returns_false(void)
{
    hf_gpio_t pin = HF_PIN(4, 0); /* only ports 0-3 exist in this fake */
    TEST_ASSERT_FALSE(hf_gpio_init(pin, HF_GPIO_OUTPUT_PUSHPULL));
}

void test_write_then_read_roundtrips(void)
{
    hf_gpio_t pin = HF_PIN(1, 5); /* port B, pin 5 */
    TEST_ASSERT_TRUE(hf_gpio_init(pin, HF_GPIO_OUTPUT_PUSHPULL));

    hf_gpio_write(pin, true);
    TEST_ASSERT_TRUE(hf_gpio_read(pin));

    hf_gpio_write(pin, false);
    TEST_ASSERT_FALSE(hf_gpio_read(pin));
}

void test_toggle_flips_state(void)
{
    hf_gpio_t pin = HF_PIN(0, 7);
    TEST_ASSERT_TRUE(hf_gpio_init(pin, HF_GPIO_OUTPUT_PUSHPULL));

    /* Start Low, toggle to High, toggle to Low */
    hf_gpio_write(pin, false);
    
    hf_gpio_toggle(pin);
    TEST_ASSERT_TRUE(hf_gpio_read(pin));  /* 1st toggle: Low -> High */
    
    hf_gpio_toggle(pin);
    TEST_ASSERT_FALSE(hf_gpio_read(pin)); /* 2nd toggle: High -> Low */

    /* Start High, toggle to Low, toggle to High */
    hf_gpio_write(pin, true);
    
    hf_gpio_toggle(pin);
    TEST_ASSERT_FALSE(hf_gpio_read(pin)); /* 3rd toggle: High -> Low */
    
    hf_gpio_toggle(pin);
    TEST_ASSERT_TRUE(hf_gpio_read(pin));  /* 4th toggle: Low -> High */
}

void test_read_reflects_externally_driven_input(void)
{
    hf_gpio_t pin = HF_PIN(2, 9); /* port C, pin 9 */
    TEST_ASSERT_TRUE(hf_gpio_init(pin, HF_GPIO_INPUT_PULLUP));

    fake_stm32_gpio_set_input(2, 9, true);
    TEST_ASSERT_TRUE(hf_gpio_read(pin));

    fake_stm32_gpio_set_input(2, 9, false);
    TEST_ASSERT_FALSE(hf_gpio_read(pin));
}

void test_write_to_invalid_pin_does_not_crash(void)
{
    hf_gpio_t pin = HF_PIN(7, 0); /* beyond the fake's 4-port table */
    hf_gpio_write(pin, true);
    hf_gpio_toggle(pin);
    TEST_ASSERT_FALSE(hf_gpio_read(pin));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_valid_pin_enables_clock_and_returns_true);
    RUN_TEST(test_init_port_beyond_table_returns_false);
    RUN_TEST(test_write_then_read_roundtrips);
    RUN_TEST(test_toggle_flips_state);
    RUN_TEST(test_read_reflects_externally_driven_input);
    RUN_TEST(test_write_to_invalid_pin_does_not_crash);
    return UNITY_END();
}
