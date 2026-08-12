/**
 * ch32v00x_it.c — 中断入口（本工程自带，替代 EVT 模板中的同名文件）
 *
 * 注意：SysTick_Handler 在 bsp/system_time.c 中定义（1ms 时基）。
 * 若把 EVT 模板的 ch32v00x_it.c 也加入工程会造成符号重复。
 */
#include "ch32v00x.h"

void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void NMI_Handler(void)
{
}

void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void)
{
    while (1) { }
}
