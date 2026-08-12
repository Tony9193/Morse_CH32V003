/**
 * test_morse.c — morse_core / morse_table 的 PC 端单元测试
 *
 * morse_core 为纯 C 模块（不依赖硬件头文件），可直接在 PC 编译验证。
 * 编译（MinGW/GCC）：
 *   gcc -Wall -Wextra -I ../User/core -I ../User/config ^
 *       test_morse.c ../User/core/morse_core.c ../User/core/morse_table.c ^
 *       -o test_morse.exe
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "morse_core.h"
#include "morse_table.h"

static uint32_t t_now;
static int      g_fail = 0;

#define CHECK(cond, msg) do {                                   \
    if (!(cond)) {                                              \
        printf("FAIL: %s (line %d)\n", (msg), __LINE__);        \
        g_fail++;                                               \
    }                                                           \
} while (0)

/* 排空事件队列，仅允许 NODE_CHANGED */
static void drain_node_changed(void)
{
    morse_event_data_t e;
    while ((e = morse_core_poll()).type != MORSE_EVT_NONE) {
        CHECK(e.type == MORSE_EVT_NODE_CHANGED, "unexpected event while draining");
    }
}

/* 按 node 的二进制位生成点划序列并拍发，返回锁定事件 */
static morse_event_data_t key_char_by_node(uint8_t node, uint16_t T)
{
    uint8_t path[8];
    int     n = 0, i, msb = 0;
    morse_event_data_t e, result;

    for (i = 15; i >= 0; i--) {
        if (node & (1u << i)) { msb = i; break; }
    }
    for (i = msb - 1; i >= 0; i--) {
        path[n++] = (uint8_t)((node >> i) & 1u);   /* 0=点 1=划 */
    }

    result.type = MORSE_EVT_NONE;
    morse_core_set_T_live(T);           /* 保证冻结的 T_char = 本测试用的 T */
    for (i = 0; i < n; i++) {
        t_now += 10u;                       /* 元素间小间隔 */
        morse_core_on_down(t_now);
        t_now += path[i] ? (uint32_t)3u * T : (uint32_t)T;
        morse_core_on_element(path[i] ? 3u * T : T, t_now);
        drain_node_changed();
    }
    t_now += 3u * T;                        /* >= 2.5T → 锁定 */
    morse_core_tick(t_now);
    while ((e = morse_core_poll()).type != MORSE_EVT_NONE) {
        if (e.type == MORSE_EVT_CHAR_LOCKED || e.type == MORSE_EVT_INVALID_CODE ||
            e.type == MORSE_EVT_NODE_OVERFLOW) {
            result = e;
        }
    }
    return result;
}

/* ---- 测试 1：36 个字符全解码 + LED 映射存在性 ---- */
static void test_all_chars(void)
{
    int node, count = 0;

    for (node = 2; node <= NODE_MAX; node++) {
        char ch = morse_table_char_of_node((uint8_t)node);
        morse_event_data_t e;
        if (ch == '?') {
            continue;
        }
        morse_core_init();
        t_now = 1000u;
        e = key_char_by_node((uint8_t)node, 100u);
        CHECK(e.type == MORSE_EVT_CHAR_LOCKED, "char should lock");
        CHECK(e.ch == ch, "locked char matches table");
        CHECK(e.node == (uint8_t)node, "locked node matches");
        CHECK(morse_table_led_of_node((uint8_t)node) >= 0, "defined char has LED");
        count++;
    }
    CHECK(count == 36, "exactly 36 defined chars (26 letters + 10 digits)");
}

/* ---- 测试 2：LED 映射抽查（对照硬件文档 §8.3） ---- */
static void test_led_map_spot(void)
{
    struct { uint8_t node; int8_t led; } cases[] = {
        { 1,  0 },   /* START = D1  */
        { 2,  1 },   /* E     = D2  */
        { 3,  2 },   /* T     = D3  */
        { 10, 9 },   /* R     = D10 */
        { 20, 18 },  /* L     = D19 */
        { 29, 26 },  /* Q     = D27 */
        { 32, 27 },  /* "5"   = D28 */
        { 33, 28 },  /* "4"   = D29 */
        { 35, 29 },  /* "3"   = D30 */
        { 39, 30 },  /* "2"   = D31 */
        { 47, 31 },  /* "1"   = D32 */
        { 48, 32 },  /* "6"   = D33 */
        { 56, 33 },  /* "7"   = D34 */
        { 60, 34 },  /* "8"   = D35 */
        { 62, 35 },  /* "9"   = D36 */
        { 63, 36 },  /* "0"   = D37 */
        { 19, -1 },  /* Ü 位无灯 */
        { 21, -1 },  /* Ä 位无灯 */
        { 30, -1 },  /* 空码无灯 */
        { 31, -1 },  /* 空码无灯 */
    };
    size_t i;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        CHECK(morse_table_led_of_node(cases[i].node) == cases[i].led,
              "NODE_TO_LED spot check");
    }
}

/* ---- 测试 3：点划判决边界（<2T 为点，>=2T 为划） ---- */
static void test_dot_dash_boundary(void)
{
    morse_event_data_t e;

    morse_core_init();
    morse_core_set_T_live(100u);
    t_now = 1000u;
    morse_core_on_down(t_now);
    t_now += 199u;
    e.type = morse_core_on_element(199u, t_now);
    CHECK(e.type == MORSE_EVT_NODE_CHANGED, "199ms accepted");
    CHECK(morse_core_get_node() == 2, "199ms (<2T) is DOT -> node 2 (E)");

    morse_core_init();
    morse_core_set_T_live(100u);
    t_now = 1000u;
    morse_core_on_down(t_now);
    t_now += 200u;
    e.type = morse_core_on_element(200u, t_now);
    CHECK(morse_core_get_node() == 3, "200ms (==2T) is DASH -> node 3 (T)");
}

/* ---- 测试 4：越界（node>63）报 OVERFLOW 并回根 ---- */
static void test_overflow(void)
{
    int i;
    morse_event_data_t e;

    morse_core_init();
    morse_core_set_T_live(100u);
    t_now = 1000u;
    for (i = 0; i < 6; i++) {             /* 6 个划：1→3→7→15→31→63→127 越界 */
        t_now += 10u;
        morse_core_on_down(t_now);
        t_now += 300u;
        e.type = morse_core_on_element(300u, t_now);
        if (i < 5) {
            drain_node_changed();
        }
    }
    CHECK(e.type == MORSE_EVT_NODE_OVERFLOW, "6th dash overflows");
    CHECK(morse_core_get_node() == 1, "overflow resets to root");
}

/* ---- 测试 5：无定义码（node 34 = "...-."）报 INVALID_CODE ---- */
static void test_invalid_code(void)
{
    morse_event_data_t e;

    morse_core_init();
    morse_core_set_T_live(100u);
    t_now = 1000u;
    e = key_char_by_node(34u, 100u);
    CHECK(e.type == MORSE_EVT_INVALID_CODE, "undefined node 34 -> INVALID_CODE");
    CHECK(morse_core_get_node() == 1, "invalid code resets to root");
}

/* ---- 测试 6：单词空格只发一次（闩锁，P1-03） ---- */
static void test_word_space_once(void)
{
    morse_event_data_t e;
    int space_count = 0;

    morse_core_init();
    morse_core_set_T_live(100u);
    t_now = 1000u;
    e = key_char_by_node(2u, 100u);       /* 拍 'E' 并锁定 */
    CHECK(e.type == MORSE_EVT_CHAR_LOCKED, "E locked");

    /* 长间隔中反复 tick：空格应只出现一次 */
    for (int i = 0; i < 20; i++) {
        t_now += 100u;                    /* 共 2000ms >= 6T=600ms */
        morse_core_tick(t_now);
        while ((e = morse_core_poll()).type != MORSE_EVT_NONE) {
            if (e.type == MORSE_EVT_WORD_SPACE) {
                space_count++;
            }
        }
    }
    CHECK(space_count == 1, "word space emitted exactly once");

    /* 新字符开始后，旧长间隔不得再补空格 */
    t_now += 10u;
    morse_core_on_down(t_now);
    t_now += 100u;
    morse_core_on_element(100u, t_now);
    drain_node_changed();
    t_now += 300u;
    morse_core_tick(t_now);
    while ((e = morse_core_poll()).type != MORSE_EVT_NONE) {
        CHECK(e.type != MORSE_EVT_WORD_SPACE, "no extra space after new char starts");
    }
}

/* ---- 测试 7：按下过程中不得提前锁定（长划保护） ---- */
static void test_no_lock_while_down(void)
{
    morse_event_data_t e;

    morse_core_init();
    morse_core_set_T_live(100u);
    t_now = 1000u;

    /* 第一个点 */
    morse_core_on_down(t_now);
    t_now += 100u;
    morse_core_on_element(100u, t_now);
    drain_node_changed();

    /* 第二个元素按下后迟迟不松手（超过 2.5T） */
    t_now += 10u;
    morse_core_on_down(t_now);
    for (int i = 0; i < 10; i++) {
        t_now += 100u;                    /* 按下中累计 1000ms > 2.5T */
        morse_core_tick(t_now);
        e = morse_core_poll();
        CHECK(e.type == MORSE_EVT_NONE, "no lock while key still down");
    }
    /* 松开为划 */
    t_now += 0u;
    morse_core_on_element(1300u, t_now);
    CHECK(morse_core_get_node() == 5, "dot then long dash -> node 5 (A)");
}

/* ---- 测试 8：字符内冻结 T_char（软件规划 §9.4 / P1-05） ---- */
static void test_T_freeze(void)
{
    morse_event_data_t e;

    morse_core_init();
    morse_core_set_T_live(100u);
    t_now = 1000u;

    /* 第一个元素按 T=100 拍点 */
    morse_core_on_down(t_now);
    t_now += 90u;
    morse_core_on_element(90u, t_now);
    drain_node_changed();

    /* 中途旋钮被拧到 T=300：判决门限与锁定门限都不应跟着变 */
    morse_core_set_T_live(300u);

    /* 第二个元素 150ms：按冻结的 T_char=100，<2T=200 → 点；
     * 若误用实时 T=300（2T=600）会误判，此处 node 应走到 4 (I) */
    t_now += 10u;
    morse_core_on_down(t_now);
    t_now += 150u;
    morse_core_on_element(150u, t_now);
    CHECK(morse_core_get_node() == 4, "T frozen: 150ms still DOT -> node 4 (I)");
    drain_node_changed();

    /* 锁定门限也按 T_char=100：松开 260ms(>=2.5T=250) 即锁定；
     * 若误用实时 T=300（2.5T=750）则此刻不会锁 */
    t_now += 260u;
    morse_core_tick(t_now);
    e = morse_core_poll();
    CHECK(e.type == MORSE_EVT_CHAR_LOCKED, "lock threshold uses frozen T_char");
    CHECK(e.ch == 'I', "locked char is I");
}

/* ---- 测试 9：CLEAR 复位 ---- */
static void test_reset(void)
{
    morse_core_init();
    morse_core_set_T_live(100u);
    t_now = 1000u;
    morse_core_on_down(t_now);
    t_now += 100u;
    morse_core_on_element(100u, t_now);
    CHECK(morse_core_get_node() == 2, "advanced to E");
    morse_core_reset();
    CHECK(morse_core_get_node() == 1, "reset back to root");
    /* 复位后残留事件应被清空 */
    CHECK(morse_core_poll().type == MORSE_EVT_NONE, "queue cleared on reset");
}

/* ---- 测试 10：BUG-1 回归 —— 锁定后 2.5T~6T 间按下新元素不得误发空格 ---- */
static void test_no_space_while_down(void)
{
    morse_event_data_t e;
    int space_count = 0;

    morse_core_init();
    morse_core_set_T_live(100u);
    t_now = 1000u;
    e = key_char_by_node(2u, 100u);       /* 拍 'E' 并锁定 */
    CHECK(e.type == MORSE_EVT_CHAR_LOCKED, "E locked (space test)");
    /* 此刻 t_now 为最后松开时刻 + 3T(300ms) */

    /* 再过 290ms（即松开后 590ms）按下新元素，随后 tick：
     * gap=600ms >= 6T，若缺 key_down 保护会误发空格 */
    t_now += 290u;
    morse_core_on_down(t_now);
    t_now += 10u;
    morse_core_tick(t_now);
    while ((e = morse_core_poll()).type != MORSE_EVT_NONE) {
        CHECK(e.type != MORSE_EVT_WORD_SPACE, "no space while new element held down");
        if (e.type == MORSE_EVT_WORD_SPACE) space_count++;
    }
    CHECK(space_count == 0, "BUG-1: no spurious space between 2.5T and 6T");

    /* 松开为点，开始新字符（清除 last_char_completed / word_space_emitted） */
    t_now += 100u;
    morse_core_on_element(100u, t_now);
    drain_node_changed();
    t_now += 300u;                        /* >= 2.5T → 锁定新字符 */
    morse_core_tick(t_now);
    while ((e = morse_core_poll()).type != MORSE_EVT_NONE) {
        CHECK(e.type == MORSE_EVT_CHAR_LOCKED, "second char locks normally");
        CHECK(e.ch == 'E', "second char is E");
    }

    /* 真正的长间隔：空格闩锁应已重新武装，仍恰好发一次 */
    for (int i = 0; i < 10; i++) {
        t_now += 100u;                    /* 共 1000ms >= 6T */
        morse_core_tick(t_now);
        while ((e = morse_core_poll()).type != MORSE_EVT_NONE) {
            if (e.type == MORSE_EVT_WORD_SPACE) space_count++;
        }
    }
    CHECK(space_count == 1, "latch re-armed: exactly one space after new char");
}

int main(void)
{
    test_all_chars();
    test_led_map_spot();
    test_dot_dash_boundary();
    test_overflow();
    test_invalid_code();
    test_word_space_once();
    test_no_lock_while_down();
    test_T_freeze();
    test_reset();
    test_no_space_while_down();

    if (g_fail == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
    printf("%d TEST(S) FAILED\n", g_fail);
    return 1;
}
