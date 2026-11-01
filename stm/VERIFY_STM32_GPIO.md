# STM32 GPIO adapter — hardware verification

Board: STM32 Nucleo-F446RE (STM32F446RE)
Toolchain: STM32CubeIDE
Adapter: `stm/gpio/hal_port_stm32.c` (unmodified)

## Verification
- [x] Host tests: `make -C stm/tests gpio && ./stm/tests/build/test_gpio` — all 6 tests pass.
- [x] Hardware: `hf_gpio_init(HF_PIN(0,5), HF_GPIO_OUTPUT_PUSHPULL)` in `main()`, `hf_gpio_toggle(HF_PIN(0,5))` + `HAL_Delay(500)` in loop. PA5 (LD2) green LED blinks at 1 Hz. 

## Integration Notes (CubeIDE)
1. Copy `hal_port_stm32.c` to `Core/Src/`
2. Copy `hal_wrapper.h` and `hal_wrapper_config.h` to `Core/Inc/`
3. In `hal_wrapper_config.h`, define `TARGET_STM32 1`
4. Include `hal_wrapper.h` in `main.c`