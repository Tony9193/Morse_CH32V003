/**
 * key_input.h — KEY 输入驱动（软件规划 §6.4 / §8.1）
 *
 * 只负责：读电平、15ms 消抖、按下/松开沿、按下时长测量。
 * 不理解点/划/字符/node（判断交给上层）。
 * 板载按键与外接电键共用 KEY 网络，软件统一处理，不做两套逻辑。
 */
#ifndef KEY_INPUT_H
#define KEY_INPUT_H

#include <stdint.h>

typedef enum {
    KEY_EVT_NONE = 0,
    KEY_EVT_DOWN,            /* 稳定按下（消抖确认） */
    KEY_EVT_UP               /* 稳定松开，duration_ms 有效 */
} key_event_t;

void        key_input_init(void);
/* 主循环每圈调用；返回一个事件。UP 事件通过 duration_ms 返回按下时长。 */
key_event_t key_input_poll(uint32_t *duration_ms);

#endif /* KEY_INPUT_H */
