/**
 * morse_core.c — 二叉树走树 + 时序判决（纯 C，无硬件依赖）
 */
#include "morse_core.h"
#include "morse_table.h"
#include "app_config.h"
#include <string.h>

#define MORSE_Q_SIZE    4u

typedef struct {
    uint8_t  node;              /* 当前节点，1~63 */
    uint8_t  char_started;      /* 当前字符已开始（至少拍过一个元素） */
    uint8_t  key_down;          /* 电键正在按下（暂停间隔判决） */
    uint16_t T_char;            /* 本字符冻结的 T（软件规划 §9.4） */
    uint16_t T_live;            /* 实时 T（ADC 更新） */
    uint32_t last_release_ms;   /* 最后一次松开时刻 */
    uint8_t  last_char_completed; /* 刚完成过字符（6T 空格判据，软件规划 §8.4） */
    uint8_t  word_space_emitted;  /* 本次长间隔空格已发（闩锁，P1-03） */
} morse_state_t;

static morse_state_t      s;
static morse_event_data_t s_q[MORSE_Q_SIZE];
static uint8_t            s_qn;

static void push(morse_event_t type, uint8_t node, char ch)
{
    if (s_qn < MORSE_Q_SIZE) {
        s_q[s_qn].type = type;
        s_q[s_qn].node = node;
        s_q[s_qn].ch   = ch;
        s_qn++;
    }
}

void morse_core_init(void)
{
    memset(&s, 0, sizeof(s));
    s.node   = NODE_ROOT;
    s.T_live = (uint16_t)((CFG_T_MIN_MS + CFG_T_MAX_MS) / 2);
    s.T_char = s.T_live;
    s_qn     = 0u;
}

void morse_core_set_T_live(uint16_t t_ms)
{
    s.T_live = t_ms;
}

void morse_core_on_down(uint32_t now_ms)
{
    (void)now_ms;
    s.key_down = 1u;    /* 按下期间禁止字符间隔判决 */
}

morse_event_t morse_core_on_element(uint32_t duration_ms, uint32_t now_ms)
{
    uint32_t threshold;
    uint8_t  dot;
    uint16_t next;

    if (!s.char_started) {
        s.T_char              = s.T_live;   /* 字符第一个元素：冻结 T */
        s.last_char_completed = 0u;
        s.word_space_emitted  = 0u;
    }
    s.char_started   = 1u;
    s.key_down       = 0u;
    s.last_release_ms = now_ms;

    /* 点/划判决：按下 < 2T → 点（软件规划 §10.1，松开时才判） */
    threshold = ((uint32_t)s.T_char * CFG_DOT_DASH_x10) / 10u;
    dot = (duration_ms < threshold) ? 1u : 0u;

    /* 走树：点 = 左子 ×2，划 = 右子 ×2+1（硬件文档 §7.2） */
    next = (uint16_t)((uint16_t)s.node * 2u + (uint16_t)(dot ? 0u : 1u));

    if (next > NODE_MAX) {
        push(MORSE_EVT_NODE_OVERFLOW, s.node, '?');
        s.node               = NODE_ROOT;
        s.char_started       = 0u;
        /* BUG 修复：越界回根后若保留上一次字符的 last_char_completed=1，
         * 而 word_space_emitted 仍为 0，tick 会在 6T 后补发一个多余空格。 */
        s.last_char_completed = 0u;
        s.word_space_emitted  = 0u;
        return MORSE_EVT_NODE_OVERFLOW;
    }

    s.node = (uint8_t)next;
    push(MORSE_EVT_NODE_CHANGED, s.node, morse_table_char_of_node(s.node));
    return MORSE_EVT_NODE_CHANGED;
}

void morse_core_tick(uint32_t now_ms)
{
    uint32_t gap;
    uint32_t limit;

    if (s.char_started) {
        if (s.key_down) {
            return;     /* 按下中：不判决（防止长划误提前锁定） */
        }
        gap   = now_ms - s.last_release_ms;
        limit = ((uint32_t)s.T_char * CFG_CHAR_GAP_x10) / 10u;
        if (gap >= limit) {
            char ch = morse_table_char_of_node(s.node);
            if (ch == '?') {
                push(MORSE_EVT_INVALID_CODE, s.node, '?');
            } else {
                push(MORSE_EVT_CHAR_LOCKED, s.node, ch);
                s.last_char_completed = 1u;
            }
            s.node        = NODE_ROOT;
            s.char_started = 0u;
        }
    } else if (s.last_char_completed && !s.word_space_emitted && !s.key_down) {
        /* BUG-1 修复：与 2.5T 分支同理，按下新元素期间暂停空格判决，
         * 防止字符锁定后 2.5T~6T 间按下电键误发单词空格 */
        gap   = now_ms - s.last_release_ms;
        limit = ((uint32_t)s.T_char * CFG_WORD_GAP_x10) / 10u;
        if (gap >= limit) {
            push(MORSE_EVT_WORD_SPACE, NODE_ROOT, ' ');
            s.word_space_emitted = 1u;      /* 闩锁：每个长间隔只发一次 */
        }
    }
}

void morse_core_reset(void)
{
    s.node                = NODE_ROOT;
    s.char_started        = 0u;
    s.key_down            = 0u;
    s.last_char_completed = 0u;
    s.word_space_emitted  = 0u;
    s_qn                  = 0u;             /* 清空未处理事件 */
}

morse_event_data_t morse_core_poll(void)
{
    morse_event_data_t e;
    uint8_t i;

    e.type = MORSE_EVT_NONE;
    e.node = 0u;
    e.ch   = 0;
    if (s_qn == 0u) {
        return e;
    }
    e = s_q[0];
    for (i = 1u; i < s_qn; i++) {
        s_q[i - 1u] = s_q[i];
    }
    s_qn--;
    return e;
}

uint8_t morse_core_get_node(void)
{
    return s.node;
}
