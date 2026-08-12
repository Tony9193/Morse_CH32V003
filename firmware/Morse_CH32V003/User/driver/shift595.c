/**
 * shift595.c — 40bit 位图 → SER/SRCLK/RCLK 时序
 */
#include "shift595.h"
#include "board_config.h"
#include "ch32v00x_gpio.h"

void shift595_write(uint64_t bits)
{
    int i;

    for (i = 39; i >= 0; i--) {                       /* 契约：bit39 先发 */
        if ((bits >> i) & 1u) {
            GPIO_SetBits(SR595_PORT, PIN_595_SER);
        } else {
            GPIO_ResetBits(SR595_PORT, PIN_595_SER);
        }
        GPIO_SetBits(SR595_PORT, PIN_595_SRCLK);
        GPIO_ResetBits(SR595_PORT, PIN_595_SRCLK);
    }
    GPIO_SetBits(SR595_PORT, PIN_595_RCLK);           /* 锁存 */
    GPIO_ResetBits(SR595_PORT, PIN_595_RCLK);
}

void shift595_enable(uint8_t on)
{
    if (on) {
        GPIO_ResetBits(SR595_PORT, PIN_595_OE);       /* OE 低有效 */
    } else {
        GPIO_SetBits(SR595_PORT, PIN_595_OE);
    }
}

void shift595_init(void)
{
    shift595_enable(0);
    shift595_write(0);
    shift595_enable(1);
}
