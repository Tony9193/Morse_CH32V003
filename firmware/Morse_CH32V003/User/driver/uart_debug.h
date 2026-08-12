/**
 * uart_debug.h — UART1 输出（软件规划 §6.10 / §12.5）
 *
 * 业务输出与调试输出分离：
 *   uart_putc/puts  —— 最终解码字符输出（RELEASE 也保留）
 *   DBG(...)        —— 调试日志（APP_DEBUG=0 时编译为空，零开销）
 *
 * 不使用 newlib printf（避免浮点库/大代码体积，软件规划 P1-08），
 * 自带迷你格式化器：支持 %d %u %x %c %s %%。
 */
#ifndef UART_DEBUG_H
#define UART_DEBUG_H

#include <stdint.h>
#include "app_config.h"

void uart_debug_init(void);
void uart_putc(char c);
void uart_puts(const char *s);
void uart_printf(const char *fmt, ...);

/* 串口命令接收（非阻塞，用于音量等运行时调参）：
 *   uart_rx_poll()  —— 每个主循环调用，累积 RXNE 字节、处理退格
 *   uart_rx_line()  —— 收到完整一行（CR/LF 结尾）返回 1 并拷出，否则 0 */
void    uart_rx_poll(void);
uint8_t uart_rx_line(char *buf, uint8_t size);

#if APP_DEBUG
#define DBG(fmt, ...) uart_printf((fmt), ##__VA_ARGS__)
#else
#define DBG(fmt, ...) ((void)0)
#endif

#endif /* UART_DEBUG_H */
