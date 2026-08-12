/**
 * system_time.c — 1ms 系统时基（SysTick 中断，软件规划原则 D：ISR 只做 tick++）
 */
#include "system_time.h"
#include "ch32v00x.h"

static volatile uint32_t s_system_ms;

void system_time_init(void)
{
    /* 已对照 EVT 官方例程 EXAM/SYSTICK/SYSTICK_Interrupt 确认（TV-7 消除）：
     * CH32V003 的 SysTick 为 WCH 自定义外设，core_riscv.h 无 SysTick_Config()，
     * 直接写寄存器配置。CTLR = 0xF：
     *   bit0 STE 使能计数 | bit1 STIE 使能中断 | bit2 STCLK=1 时钟取 HCLK |
     *   bit3 STRE=1 自动重装（到 CMP 后 CNT 清零重装，周期 = CMP+1 拍） */
    NVIC_EnableIRQ(SysTick_IRQn);
    SysTick->SR   = 0u;
    SysTick->CMP  = (SystemCoreClock / 1000u) - 1u;   /* 1ms 周期 */
    SysTick->CNT  = 0u;
    SysTick->CTLR = 0xFu;
}

uint32_t now_ms(void)
{
    return s_system_ms;
}

uint32_t elapsed_ms(uint32_t start)
{
    /* 无符号减法天然处理 49.7 天回绕；
     * 禁止写成 now > start + timeout 的形式（软件规划 §9.2）。 */
    return (uint32_t)(now_ms() - start);
}

/* 中断入口名与属性与 EVT 官方例程一致（WCH-Interrupt-fast 快速中断）。
 * 注意：勿将 EVT 模板的 ch32v00x_it.c 加入工程，避免重复定义。 */
void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void SysTick_Handler(void)
{
    s_system_ms++;
    SysTick->SR = 0;      /* 清中断标志 */
}
