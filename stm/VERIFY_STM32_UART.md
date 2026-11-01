# STM32 UART adapter — hardware verification

Board: STM32 Nucleo-F446RE (STM32F446RE)
Toolchain: STM32CubeIDE
Adapter: `stm/uart/hal_port_stm32_uart.c` (unmodified)

## Verification
- [x] Host tests: `make -C stm/tests uart && ./stm/tests/build/test_uart` — all 7 tests pass.
- [x] Hardware: Blocking TX (`hf_uart_write`) and Async TX (`hf_uart_write_async`) verified via ST-LINK VCP (USART2) at 115200 baud.
- [x] Background RX: Verified by typing data into the serial terminal and reading it back via the ring buffer (`hf_uart_read`).

## Integration Notes (CubeIDE)
1. Enable USART1, USART2, and USART3 as Asynchronous in CubeMX. (The adapter `extern`s all three handles).
2. Enable the **USART2 global interrupt** in NVIC settings (required for async callbacks).
3. Copy `hal_port_stm32_uart.c` into `Core/Src/`.
4. Call `hf_uart_init(1, 115200)` in `main.c` to initialize USART2.