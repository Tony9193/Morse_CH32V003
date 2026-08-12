/**
 * clear_input.c — CLEAR 状态机（软件规划 §8.2）
 *
 *   IDLE --按下(消抖后)--> DOWN
 *   DOWN --持续≥1s--> 发 LONG，进入 LONG_SENT
 *   DOWN --松开--> 发 SHORT，回 IDLE
 *   LONG_SENT --松开--> IDLE（不补发 SHORT）
 */
#include "clear_input.h"
#include "board_config.h"
#include "app_config.h"
#include "system_time.h"
#include "ch32v00x_gpio.h"

typedef enum { CI_IDLE, CI_DOWN, CI_LONG_SENT } ci_state_t;

static ci_state_t s_state;
static uint8_t    s_last_raw;        /* 上一次采样（0 = 未按下） */
static uint32_t   s_change_ms;       /* 电平最后变化时刻 */
static uint8_t    s_stable_pressed;  /* 消抖后的稳定状态 */
static uint32_t   s_down_ms;

static uint8_t raw_pressed(void)
{
    return (GPIO_ReadInputDataBit(CLEAR_PORT, CLEAR_PIN) == Bit_RESET) ? 1u : 0u;
}

void clear_input_init(void)
{
    s_state          = CI_IDLE;
    s_last_raw       = raw_pressed();
    s_change_ms      = now_ms();
    s_stable_pressed = 0u;
}

clear_event_t clear_input_poll(void)
{
    uint32_t now = now_ms();
    uint8_t  raw = raw_pressed();
    uint8_t  pressed_edge = 0u;
    uint8_t  released_edge = 0u;

    /* --- 消抖：电平持续 >= CFG_CLEAR_DEBOUNCE_MS 才更新稳定态 --- */
    if (raw != s_last_raw) {
        s_last_raw  = raw;
        s_change_ms = now;
    }
    if (elapsed_ms(s_change_ms) >= CFG_CLEAR_DEBOUNCE_MS) {
        if (raw && !s_stable_pressed) {
            s_stable_pressed = 1u;
            pressed_edge = 1u;
        } else if (!raw && s_stable_pressed) {
            s_stable_pressed = 0u;
            released_edge = 1u;
        }
    }

    /* --- 短按 / 长按判定 --- */
    switch (s_state) {
    case CI_IDLE:
        if (pressed_edge) {
            s_state   = CI_DOWN;
            s_down_ms = now;
        }
        break;
    case CI_DOWN:
        if (released_edge) {
            s_state = CI_IDLE;
            return CLEAR_EVT_SHORT;
        }
        if (elapsed_ms(s_down_ms) >= CFG_CLEAR_LONG_MS) {
            s_state = CI_LONG_SENT;
            return CLEAR_EVT_LONG;
        }
        break;
    case CI_LONG_SENT:
        if (released_edge) {
            s_state = CI_IDLE;          /* 不补发 SHORT */
        }
        break;
    default:
        s_state = CI_IDLE;
        break;
    }
    return CLEAR_EVT_NONE;
}
