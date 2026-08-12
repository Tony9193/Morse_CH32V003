/**
 * clear_input.h — CLEAR 功能键驱动（软件规划 §6.5 / §8.2）
 *
 * 短按：回根/清空；长按 ≥1s：切换显示模式。
 * 关键约束：一次长按只能产生一次 LONG，且松手后不得补发 SHORT。
 */
#ifndef CLEAR_INPUT_H
#define CLEAR_INPUT_H

typedef enum {
    CLEAR_EVT_NONE = 0,
    CLEAR_EVT_SHORT,
    CLEAR_EVT_LONG
} clear_event_t;

void          clear_input_init(void);
clear_event_t clear_input_poll(void);   /* 主循环每圈调用 */

#endif /* CLEAR_INPUT_H */
