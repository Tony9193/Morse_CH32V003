/**
 * board.c — 板级初始化（初始化顺序见软件规划 §13）
 */
#include "board.h"
#include "board_config.h"
#include "ch32v00x_gpio.h"
#include "ch32v00x_rcc.h"
#include "shift595.h"

void board_init(void)
{
    GPIO_InitTypeDef g;

    /* 1. 端口时钟（PA/PC/PD） */
    RCC_APB2PeriphClockCmd(KEY_RCC_CLK | SR595_RCC_CLK | BUZZER_RCC_CLK, ENABLE);

    /* 2. 595 OE 先置高 = 禁止输出，防止初始化期间 LED 乱亮（软件规划 §13.1，OE 低有效勿写反） */
    GPIO_StructInit(&g);
    g.GPIO_Pin   = PIN_595_OE;
    g.GPIO_Mode  = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SR595_PORT, &g);
    GPIO_SetBits(SR595_PORT, PIN_595_OE);

    /* 3. SER/SRCLK/RCLK：推挽输出，空闲低 */
    g.GPIO_Pin = PIN_595_SER | PIN_595_SRCLK | PIN_595_RCLK;
    GPIO_Init(SR595_PORT, &g);
    GPIO_ResetBits(SR595_PORT, PIN_595_SER | PIN_595_SRCLK | PIN_595_RCLK);

    /* 4. KEY / CLEAR：输入上拉，按下接地为低 */
    g.GPIO_Pin  = KEY_PIN;
    g.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(KEY_PORT, &g);
    g.GPIO_Pin = CLEAR_PIN;
    GPIO_Init(CLEAR_PORT, &g);

    /* 5. 595 上电序列：移位全 0 并锁存 → 使能输出 */
    shift595_write(0);
    shift595_enable(1);
}
