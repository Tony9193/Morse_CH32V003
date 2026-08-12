/**
 * key_input.c — 四状态消抖状态机（软件规划 §8.1）
 *
 *   RELEASED --raw low--> PRESS_DEBOUNCE --连续低≥15ms--> PRESSED
 *   PRESSED  --raw high-> RELEASE_DEBOUNCE --连续高≥15ms--> RELEASED
 *
 * 只有 PRESS_DEBOUNCE→PRESSED 产生 KEY_DOWN；
 * 只有 RELEASE_DEBOUNCE→RELEASED 产生 KEY_UP（携带按下时长）。
 * 事件队列深度 4：即使主循环短暂繁忙也不丢沿。
 */
#include "key_input.h"
#include "board_config.h"
#include "app_config.h"
#include "system_time.h"
#include "ch32v00x_gpio.h"

typedef enum { KS_RELEASED, KS_PRESS_DEB, KS_PRESSED, KS_REL_DEB } key_state_t;

typedef struct {
    key_event_t evt;
    uint32_t    dur;
} key_item_t;

static key_state_t s_state;
static uint32_t    s_deb_ms;        /* 进入消抖态的时刻 */
static uint32_t    s_down_ms;       /* 确认按下的时刻 */
static key_item_t  s_q[4];
static uint8_t     s_qn;

static uint8_t raw_pressed(void)
{
    return (GPIO_ReadInputDataBit(KEY_PORT, KEY_PIN) == Bit_RESET) ? 1u : 0u;
}

void key_input_init(void)
{
    s_state = KS_RELEASED;
    s_qn    = 0;
}

static void push(key_event_t e, uint32_t dur)
{
    if (s_qn < 4u) {
        s_q[s_qn].evt = e;
        s_q[s_qn].dur = dur;
        s_qn++;
    }
}

key_event_t key_input_poll(uint32_t *duration_ms)
{
    uint32_t now = now_ms();
    uint8_t  raw = raw_pressed();

    switch (s_state) {
    case KS_RELEASED:
        if (raw) { s_state = KS_PRESS_DEB; s_deb_ms = now; }
        break;
    case KS_PRESS_DEB:
        if (!raw) {
            s_state = KS_RELEASED;                       /* 抖动，回落 */
        } else if (elapsed_ms(s_deb_ms) >= CFG_KEY_DEBOUNCE_MS) {
            s_state   = KS_PRESSED;
            s_down_ms = now;
            push(KEY_EVT_DOWN, 0u);
        }
        break;
    case KS_PRESSED:
        if (!raw) { s_state = KS_REL_DEB; s_deb_ms = now; }
        break;
    case KS_REL_DEB:
        if (raw) {
            s_state = KS_PRESSED;                        /* 抖动，回落 */
        } else if (elapsed_ms(s_deb_ms) >= CFG_KEY_DEBOUNCE_MS) {
            s_state = KS_RELEASED;
            push(KEY_EVT_UP, elapsed_ms(s_down_ms));
        }
        break;
    default:
        s_state = KS_RELEASED;
        break;
    }

    if (duration_ms != 0) {
        *duration_ms = 0u;
    }
    if (s_qn == 0u) {
        return KEY_EVT_NONE;
    }
    {
        key_item_t it = s_q[0];
        uint8_t i;
        for (i = 1u; i < s_qn; i++) { s_q[i - 1u] = s_q[i]; }
        s_qn--;
        if (duration_ms != 0) {
            *duration_ms = it.dur;
        }
        return it.evt;
    }
}
