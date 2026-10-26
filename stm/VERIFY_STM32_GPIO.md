# STM32 GPIO adapter — hardware verification

Board: STM32 Nucleo-F446RE (STM32F446RE)
Toolchain: STM32CubeIDE, CubeMX-generated GPIO-only project
Adapter: `stm/gpio/hal_port_stm32.c` (unmodified)
Defines in main.h: `TARGET_STM32 1`, `HF_ENABLE_GPIO 1`

## Result
- Host tests: `make -C stm/tests gpio && ./stm/tests/build/test_gpio` — **all 6 tests pass**.
- Hardware: `hf_gpio_init(HF_PIN(0,5), HF_GPIO_OUTPUT_PUSHPULL)` in `main()`, `hf_gpio_toggle(HF_PIN(0,5))` + `HAL_Delay(500)` in loop. PA5 (LD2) green LED blinks at 1 Hz. 

## Integration Steps (CubeIDE)
1. Copy `hal_port_stm32.c` to `Core/Src/`
2. Copy `hal_wrapper.h` and `hal_wrapper_config.h` to `Core/Inc/`
3. In `hal_wrapper_config.h`, define `TARGET_STM32 1`
4. Include `hal_wrapper.h` in `main.c`

## Note on `hf_gpio_toggle` atomicity
Verified current STM32 HAL implements `HAL_GPIO_TogglePin()` via a single BSRR store (see `stm32f4xx_hal_gpio.c`), not `ODR ^= mask`. The adapter inherits correct per-pin atomic output transitions. No fix needed.

Known limitation: ODR read + BSRR write pair is not atomic, so two contexts toggling the same pin can lose a toggle. STM32 has no toggle register; not fixable in software without a lock.
