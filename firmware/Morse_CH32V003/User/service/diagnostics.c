/**
 * diagnostics.c — BRINGUP 模式主流程（仅测试用途，可含忙等待）
 *
 * 操作说明：
 *  - 上电自动流水灯一遍（D1→D37，随后全亮/全灭）
 *  - KEY：UART 打印 DOWN/UP 与按下时长（对拍发节奏/消抖的验收）
 *  - ADC：每 500ms 打印 raw 与 T（旋转电位器应单调、覆盖 60~300ms）
 *  - CLEAR 短按：全灯开/关；CLEAR 长按：重放流水灯
 */
#include "diagnostics.h"
#include "led_tree.h"
#include "key_input.h"
#include "clear_input.h"
#include "adc_speed.h"
#include "uart_debug.h"
#include "system_time.h"
#include "app_config.h"

static void wait_ms(uint32_t ms)
{
    uint32_t t = now_ms();
    while ((now_ms() - t) < ms) { }
}

static void run_chaser(void)
{
    uint8_t i;

    uart_puts("CHASER D1..D37\r\n");
    for (i = 0u; i < LED_COUNT; i++) {
        led_tree_show_raw((uint64_t)1 << i);
        wait_ms(120u);
    }
    led_tree_show_raw(LED_ALL_MASK);
    wait_ms(600u);
    led_tree_show_raw(0u);
    wait_ms(300u);
}

void diagnostics_run(void)
{
    uint32_t next_adc = 0u;
    uint8_t  all_on   = 0u;

    uart_puts("\r\n=== Morse CH32V003 BRINGUP ===\r\n");
    run_chaser();

    for (;;) {
        uint32_t    now = now_ms();
        uint32_t    dur = 0u;
        key_event_t ke;
        clear_event_t ce;

        ke = key_input_poll(&dur);
        if (ke == KEY_EVT_DOWN) {
            uart_puts("KEY DOWN\r\n");
        } else if (ke == KEY_EVT_UP) {
            uart_printf("KEY UP dur=%u ms\r\n", (unsigned)dur);
        }

        ce = clear_input_poll();
        if (ce == CLEAR_EVT_SHORT) {
            all_on = (uint8_t)!all_on;
            led_tree_show_raw(all_on ? LED_ALL_MASK : 0u);
            uart_puts(all_on ? "ALL ON\r\n" : "ALL OFF\r\n");
        } else if (ce == CLEAR_EVT_LONG) {
            run_chaser();
        }

        adc_speed_update(now);
        if ((int32_t)(now - next_adc) >= 0) {
            next_adc = now + 500u;
            uart_printf("ADC raw=%u T=%u ms\r\n",
                        (unsigned)adc_speed_get_raw(), (unsigned)adc_speed_get_T());
        }
    }
}
