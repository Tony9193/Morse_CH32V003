# Vendor/ — WCH 官方文件放置处

从 CH32V003 EVT 官方例程包复制以下文件到本目录（保持原样，不修改）：

```text
Vendor/
├─ startup_ch32v00x.S          启动文件
├─ ch32v00x.h                  寄存器定义
├─ core_riscv.h                内核头（含 SysTick_Config）
├─ ch32v00x_conf.h             外设头汇总（若工程模板需要）
├─ system_ch32v00x.c/.h        系统时钟初始化（SystemInit / SystemCoreClock）
└─ peripheral/
   ├─ ch32v00x_gpio.c/.h
   ├─ ch32v00x_rcc.c/.h
   ├─ ch32v00x_usart.c/.h
   ├─ ch32v00x_adc.c/.h
   ├─ ch32v00x_tim.c/.h
   └─ ch32v00x_misc.c/.h       NVIC 辅助
```

注意：

1. **不要**把 EVT 模板的 `User/main.c` 与 `User/ch32v00x_it.c` 加入工程——本工程自带这两个文件
   （本工程的 `User/ch32v00x_it.c` 提供 NMI/HardFault 入口；SysTick_Handler 在 `User/bsp/system_time.c`）。
2. EVT 中的 Debug 调试串口代码（`debug.c`）可不引入，本工程自带 uart_debug。
3. 官方文件与芯片行为以 EVT 为准；发现本工程代码与 EVT 冲突时，改本工程、不改 Vendor。
