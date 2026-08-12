/**
 * morse_table.h — 莫尔斯树数据表（软件规划 §6.11）
 *
 * 映射表是"数据"，不散落在算法里。本文件与 morse_table.c 为纯 C，
 * 不依赖任何硬件头文件。
 */
#ifndef MORSE_TABLE_H
#define MORSE_TABLE_H

#include <stdint.h>

#define NODE_ROOT   1       /* START */
#define NODE_MAX    63      /* 5 层树最大节点号；>63 即越界 */

/* 节点 → 字符；无定义节点返回 '?'（含 node 0 与越界保护） */
char morse_table_char_of_node(uint8_t node);

/* 节点 → LED 序号（0~36）；无灯节点返回 -1（node 19/21/30/31 及未定义数字位） */
int8_t morse_table_led_of_node(uint8_t node);

#endif /* MORSE_TABLE_H */
