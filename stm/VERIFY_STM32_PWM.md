# STM32 PWM adapter — hardware verification

Board: STM32 Nucleo-F446RE (STM32F446RE)
Toolchain: STM32CubeIDE
Adapter: `stm/pwm/hal_port_stm32_pwm.c` (unmodified)

## Verification
- [x] Host tests: `make -C stm/tests pwm && ./stm/tests/build/test_pwm` — all 6 tests pass.
- [x] Hardware: Faded the green LED (PA5) from 0% to 100% and back, verifying `hf_pwm_init`, `hf_pwm_start`, and `hf_pwm_set_duty`.

## Integration Notes (CubeIDE)
1. **Handles:** The adapter `extern`s both `htim2` and `htim3`. If you only enable one in CubeMX, you will get an `undefined reference` linker error. Enable both as PWM Generation.
2. **Pin Mapping:** The Nucleo's green LED is on PA5. You must manually map PA5 to `TIM2_CH1` in CubeMX (unpin the default `GPIO_Output`).
3. **Prescaler:** The adapter assumes a 1 MHz tick. With a 90 MHz APB1 timer clock, set the TIM2 Prescaler to `89`.
4. **Code:** Call `hf_pwm_init(HF_PWM(0, 0), 1000)` and `hf_pwm_start(HF_PWM(0, 0))` in `main.c`.