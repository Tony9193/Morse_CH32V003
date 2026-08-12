/**
 * led_tree.c — node → LED 位图 → shift595
 *
 * 注意：node 编号 ≠ LED 编号（数字节点不连续），必须经 NODE_TO_LED 查表
 * （软件规划 §11.1）。路径模式回溯父节点时跳过无灯节点（硬件文档 §13.2）。
 */
#include "led_tree.h"
#include "morse_table.h"
#include "shift595.h"
#include "app_config.h"

typedef enum {
    ANIM_IDLE,
    ANIM_LOCK_HOLD,     /* 锁定反馈：常亮保持 */
    ANIM_LOCK_BLINK,    /* 锁定反馈：闪烁 */
    ANIM_ERROR          /* 错误反馈：全树闪烁 */
} anim_t;

static led_mode_t s_mode      = LED_MODE_SINGLE;
static uint8_t    s_cur_node  = NODE_ROOT;
static anim_t     s_anim      = ANIM_IDLE;
static uint8_t    s_anim_node;
static uint32_t   s_anim_start;
static uint32_t   s_anim_phase;

static uint64_t frame_for_node(uint8_t node)
{
    uint64_t f = 0u;

    if (s_mode == LED_MODE_SINGLE) {
        int8_t i = morse_table_led_of_node(node);
        if (i >= 0) {
            f = (uint64_t)1 << i;
        }
    } else {
        uint8_t n = node;
        while (n >= 1u) {                     /* 逐层回溯至根 */
            int8_t i = morse_table_led_of_node(n);
            if (i >= 0) {
                f |= (uint64_t)1 << i;        /* 无灯节点(19/21/30/31等)跳过 */
            }
            n >>= 1;
        }
    }
    return f;
}

static void finish_animation(void)
{
    s_anim     = ANIM_IDLE;
    s_cur_node = NODE_ROOT;
    shift595_write(frame_for_node(NODE_ROOT));    /* 动画结束回 START */
}

void led_tree_init(void)
{
    s_mode     = LED_MODE_SINGLE;
    s_cur_node = NODE_ROOT;
    s_anim     = ANIM_IDLE;
    shift595_write(frame_for_node(NODE_ROOT));
}

void led_tree_set_mode(led_mode_t m)
{
    s_mode = m;
    if (s_anim == ANIM_IDLE) {
        shift595_write(frame_for_node(s_cur_node));   /* 立即按新模式重绘 */
    }
}

led_mode_t led_tree_get_mode(void)
{
    return s_mode;
}

void led_tree_toggle_mode(void)
{
    led_tree_set_mode((s_mode == LED_MODE_SINGLE) ? LED_MODE_PATH : LED_MODE_SINGLE);
}

void led_tree_show_node(uint8_t node)
{
    s_anim     = ANIM_IDLE;                 /* 新输入优先：取消进行中动画 */
    s_cur_node = node;
    shift595_write(frame_for_node(node));
}

void led_tree_lock_feedback(uint8_t node, uint32_t now)
{
    s_anim_node  = node;
    s_anim_start = now;
    s_anim_phase = 0xFFFFFFFFu;             /* 强制首帧重绘 */

#if (CFG_LOCK_FEEDBACK_STYLE) == (CFG_LOCK_STYLE_BLINK)
    s_anim = ANIM_LOCK_BLINK;
#else
    {
        int8_t i = morse_table_led_of_node(node);
        s_anim = ANIM_LOCK_HOLD;
        shift595_write((i >= 0) ? ((uint64_t)1 << i) : 0u);
    }
#endif
}

void led_tree_error_feedback(uint32_t now)
{
    s_anim       = ANIM_ERROR;
    s_anim_start = now;
    s_anim_phase = 0xFFFFFFFFu;
}

void led_tree_cancel_animation(void)
{
    s_anim = ANIM_IDLE;
}

void led_tree_show_raw(uint64_t bits)
{
    s_anim = ANIM_IDLE;
    shift595_write(bits);
}

void led_tree_update(uint32_t now)
{
    uint32_t el;
    uint32_t ph;
    uint32_t total;

    switch (s_anim) {
    case ANIM_LOCK_HOLD:
        if ((now - s_anim_start) >= CFG_LOCK_HOLD_MS) {
            finish_animation();
        }
        break;

    case ANIM_LOCK_BLINK: {
        int8_t i = morse_table_led_of_node(s_anim_node);
        total = (uint32_t)CFG_LOCK_BLINK_TIMES * 2u * CFG_LOCK_BLINK_HALF_MS;
        if ((now - s_anim_start) >= total) {
            finish_animation();
            break;
        }
        ph = (now - s_anim_start) / CFG_LOCK_BLINK_HALF_MS;
        if (ph != s_anim_phase) {
            s_anim_phase = ph;
            shift595_write(((ph & 1u) == 0u && i >= 0) ? ((uint64_t)1 << i) : 0u);
        }
        break;
    }

    case ANIM_ERROR:
        total = (uint32_t)CFG_ERR_BLINK_TIMES * 2u * CFG_ERR_BLINK_HALF_MS;
        el    = now - s_anim_start;
        if (el >= total) {
            finish_animation();
            break;
        }
        ph = el / CFG_ERR_BLINK_HALF_MS;
        if (ph != s_anim_phase) {
            s_anim_phase = ph;
            shift595_write(((ph & 1u) == 0u) ? LED_ALL_MASK : 0u);
        }
        break;

    case ANIM_IDLE:
    default:
        break;
    }
}
