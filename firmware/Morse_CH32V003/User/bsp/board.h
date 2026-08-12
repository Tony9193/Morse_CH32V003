/**
 * board.h — 板级初始化入口（软件规划 §6.2）
 */
#ifndef BOARD_H
#define BOARD_H

/* 初始化：RCC 时钟、595 控制 GPIO（OE 先禁用）、KEY/CLEAR 上拉输入、
 * 595 上电清零后使能输出。
 * UART/ADC/蜂鸣器由各自驱动模块初始化（app_controller 按文档 §13 顺序调用）。
 * PD1(SWIO)/PD7(NRST) 永不初始化。 */
void board_init(void);

#endif /* BOARD_H */
