/**
 * led_tree.h — LED 树显示服务（软件规划 §6.8 / §11）
 *
 * 输入：node / 显示模式 / 锁定 / 错误状态；输出：40bit led_state → shift595。
 * 所有动画（锁定反馈、错误闪烁）均为非阻塞状态机，由 led_tree_update() 驱动。
 */
#ifndef LED_TREE_H
#define LED_TREE_H

#include <stdint.h>

#define LED_COUNT       37u
#define LED_ALL_MASK    (((uint64_t)1 << LED_COUNT) - 1u)   /* D1~D37 */

typedef enum {
    LED_MODE_SINGLE,    /* 单点模式（默认）：只有当前节点亮 */
    LED_MODE_PATH       /* 路径模式：START → 当前节点整条路径亮 */
} led_mode_t;

void       led_tree_init(void);
void       led_tree_set_mode(led_mode_t m);
led_mode_t led_tree_get_mode(void);
void       led_tree_toggle_mode(void);

void led_tree_show_node(uint8_t node);               /* node 变化时刷新（取消进行中动画） */
void led_tree_lock_feedback(uint8_t node, uint32_t now);  /* 字符锁定反馈 */
void led_tree_error_feedback(uint32_t now);          /* 错误：全树闪 3 次 */
void led_tree_cancel_animation(void);                /* 新输入到来时取消动画 */
void led_tree_show_raw(uint64_t bits);               /* 诊断用：直接显示位图 */
void led_tree_update(uint32_t now);                  /* 主循环每圈调用 */

#endif /* LED_TREE_H */
