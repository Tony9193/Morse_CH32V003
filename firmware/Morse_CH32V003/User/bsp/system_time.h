/**
 * system_time.h — 全系统唯一 1ms 时间基准（软件规划 §6.3 / §9）
 *
 * 所有计时（消抖/按下时长/字符间隔/单词间隔/长按/动画/ADC 调度）
 * 一律使用 now_ms()/elapsed_ms()，禁止模块自造计时。
 */
#ifndef SYSTEM_TIME_H
#define SYSTEM_TIME_H

#include <stdint.h>

void     system_time_init(void);          /* SysTick 配置为 1ms 中断 */
uint32_t now_ms(void);                    /* 当前毫秒计数 */
uint32_t elapsed_ms(uint32_t start);      /* now - start，无符号回绕安全（软件规划 §9.2） */

#endif /* SYSTEM_TIME_H */
