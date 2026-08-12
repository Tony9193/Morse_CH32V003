/**
 * morse_core.h — 纯莫尔斯解析核心（软件规划 §6.12 / Layer 5）
 *
 * 本模块是纯 C：不引用 GPIO、WCH 外设库或任何板级头文件（可 PC 单测）。
 *
 * 输入：dot/dash（由按下时长换算，见 on_element）、时间、复位
 * 输出：事件队列（node 变化 / 字符锁定 / 非法码 / 越界 / 单词空格）
 *
 * 关键设计（软件规划 §9.4 / §10 / §17）：
 *  - 字符内冻结 T_char：一个字符进行中不随电位器变化改变判定标准；
 *  - 按下过程中不提前判决：锁定时序只在松开后计时，且需 key_down 标志保护，
 *    防止长划按下期间 gap 误到达 2.5T 提前锁字符；
 *  - 单词空格闩锁：每个长间隔只输出一次空格（word_space_emitted）。
 */
#ifndef MORSE_CORE_H
#define MORSE_CORE_H

#include <stdint.h>

typedef enum {
    MORSE_EVT_NONE = 0,
    MORSE_EVT_NODE_CHANGED,     /* node 前进了一步（携带新 node） */
    MORSE_EVT_CHAR_LOCKED,      /* 有效字符锁定（携带字符） */
    MORSE_EVT_INVALID_CODE,     /* 锁定到无定义字符的节点 */
    MORSE_EVT_NODE_OVERFLOW,    /* node > 63，全树报错并回根 */
    MORSE_EVT_WORD_SPACE        /* 单词间隔到，输出空格 */
} morse_event_t;

typedef struct {
    morse_event_t type;
    uint8_t       node;
    char          ch;
} morse_event_data_t;

void          morse_core_init(void);
void          morse_core_set_T_live(uint16_t t_ms);      /* ADC 层持续更新的实时 T */
void          morse_core_on_down(uint32_t now_ms);       /* KEY 稳定按下：暂停间隔计时 */
/* KEY 稳定松开：按时长判点/划并推进 node。duration_ms = 按下时长 */
morse_event_t morse_core_on_element(uint32_t duration_ms, uint32_t now_ms);
void          morse_core_tick(uint32_t now_ms);          /* 检查 2.5T 锁定 / 6T 空格 */
void          morse_core_reset(void);                    /* CLEAR 短按：回根、清状态 */
morse_event_data_t morse_core_poll(void);                /* 取一个待处理事件 */
uint8_t       morse_core_get_node(void);

#endif /* MORSE_CORE_H */
