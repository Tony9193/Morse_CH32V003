/**
 * app_controller.c — 系统调度：初始化序列 + 主循环事件分发
 */
#include "app_controller.h"
#include "app_config.h"
#include "board.h"
#include "system_time.h"
#include "shift595.h"
#include "key_input.h"
#include "clear_input.h"
#include "adc_speed.h"
#include "buzzer.h"
#include "uart_debug.h"
#include "morse_core.h"
#include "morse_table.h"
#include "led_tree.h"
#include "error_manager.h"

void app_controller_init(void)
{
    /* 初始化顺序按软件规划 §13：
     * 1 系统时钟(启动文件 SystemInit) → 2 时基 → 3 GPIO → 4 UART →
     * 5-7 595(GPIO/OE 安全/清零) → 8 KEY/CLEAR → 9 ADC → 10 蜂鸣器 →
     * 11 莫尔斯核心 → 12 LED 服务 → 13 自检(可选) → 14 主循环 */
    system_time_init();
    board_init();
    uart_debug_init();
    key_input_init();
    clear_input_init();
    adc_speed_init();
    buzzer_init();
    buzzer_off();
    morse_core_init();
    led_tree_init();

    uart_puts("\r\nMorse CH32V003 boot\r\n");
    DBG("T=%u ms mode=%s volume=%u%% (serial: V<n>)\r\n",
        (unsigned)adc_speed_get_T(),
        (led_tree_get_mode() == LED_MODE_PATH) ? "PATH" : "SINGLE",
        (unsigned)buzzer_get_volume());
}

static void handle_key(void)
{
    uint32_t    now = now_ms();
    uint32_t    dur = 0u;
    key_event_t ke  = key_input_poll(&dur);

    if (ke == KEY_EVT_DOWN) {
        buzzer_on();                    /* 侧音（软件规划 §12.4） */
        morse_core_on_down(now);        /* 暂停间隔判决 */
        DBG("KEY DOWN\r\n");
    } else if (ke == KEY_EVT_UP) {
        buzzer_off();
        DBG("KEY UP dur=%u ms T=%u\r\n", (unsigned)dur, (unsigned)adc_speed_get_T());
        morse_core_on_element(dur, now);
    }
}

static void handle_clear(void)
{
    clear_event_t ce = clear_input_poll();

    if (ce == CLEAR_EVT_SHORT) {
        morse_core_reset();
        led_tree_cancel_animation();
        led_tree_show_node(NODE_ROOT);
        DBG("CLEAR: back to root\r\n");
    } else if (ce == CLEAR_EVT_LONG) {
        led_tree_toggle_mode();
        DBG("MODE -> %s\r\n",
            (led_tree_get_mode() == LED_MODE_PATH) ? "PATH" : "SINGLE");
    }
}

static void handle_morse_events(void)
{
    morse_event_data_t me;

    while ((me = morse_core_poll()).type != MORSE_EVT_NONE) {
        switch (me.type) {
        case MORSE_EVT_NODE_CHANGED:
            led_tree_cancel_animation();        /* 新输入打断锁定反馈动画 */
            led_tree_show_node(me.node);
            DBG("node=%u\r\n", (unsigned)me.node);
            break;

        case MORSE_EVT_CHAR_LOCKED:
            uart_putc(me.ch);                   /* 业务输出：解码字符（RELEASE 保留） */
#if APP_DEBUG
            uart_puts("\r\n");
#endif
            DBG("LOCK '%c' (node=%u)\r\n", me.ch, (unsigned)me.node);
            led_tree_lock_feedback(me.node, now_ms());
            break;

        case MORSE_EVT_INVALID_CODE:
            DBG("invalid code at node=%u\r\n", (unsigned)me.node);
            error_manager_report(ERROR_INVALID_CODE);
            break;

        case MORSE_EVT_NODE_OVERFLOW:
            error_manager_report(ERROR_NODE_OVERFLOW);
            break;

        case MORSE_EVT_WORD_SPACE:
            uart_putc(' ');                     /* 每个长间隔只发一次（核心内闩锁） */
            break;

        default:
            break;
        }
    }
}

/* 串口运行时调参（音量）：V<n> 设置 0~100，V? 查询，? 帮助 */
static uint8_t parse_volume_arg(const char *s, uint8_t *out)
{
    uint16_t v = 0u;

    if (*s == '\0') {
        return 0u;
    }
    while (*s) {
        if (*s < '0' || *s > '9') {
            return 0u;
        }
        v = (uint16_t)(v * 10u + (uint16_t)(*s - '0'));
        if (v > 100u) {
            return 0u;
        }
        s++;
    }
    *out = (uint8_t)v;
    return 1u;
}

static void handle_serial(void)
{
    char    line[CFG_UART_RX_LINE_MAX];
    uint8_t vol;

    uart_rx_poll();
    if (uart_rx_line(line, (uint8_t)sizeof(line)) == 0u) {
        return;
    }

    if (line[0] == 'V' || line[0] == 'v') {
        if (line[1] == '?') {
            uart_printf("VOL=%u%%\r\n", (unsigned)buzzer_get_volume());
        } else if (parse_volume_arg(&line[1], &vol)) {
            buzzer_set_volume(vol);
            uart_printf("VOL=%u%%\r\n", (unsigned)vol);
        } else {
            uart_puts("usage: V<0-100> | V?\r\n");
        }
    } else if (line[0] == '?') {
        uart_puts("commands: V<0-100> volume, V? query\r\n");
    } else {
        uart_puts("?\r\n");
    }
}

void app_controller_run(void)
{
    uint32_t now = now_ms();

    /* 电位器 → T（驱动内部自行节流 100ms） */
    adc_speed_update(now);
    morse_core_set_T_live(adc_speed_get_T());

    handle_key();
    handle_clear();
    handle_serial();                /* 串口命令：音量等运行时调参 */

    /* 时序判决：2.5T 锁定 / 6T 空格 */
    morse_core_tick(now_ms());

    handle_morse_events();

    /* 非阻塞动画推进 */
    led_tree_update(now_ms());
}
