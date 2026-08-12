/**
 * uart_debug.c — USART1 / PD5(TX) PD6(RX) / 115200 8N1
 */
#include <stdarg.h>
#include <stdint.h>
#include "uart_debug.h"
#include "board_config.h"
#include "ch32v00x_gpio.h"
#include "ch32v00x_rcc.h"
#include "ch32v00x_usart.h"

void uart_debug_init(void)
{
    GPIO_InitTypeDef  g;
    USART_InitTypeDef u;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | UART_RCC_CLK, ENABLE);

    g.GPIO_Pin   = UART_TX_PIN;
    g.GPIO_Mode  = GPIO_Mode_AF_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(UART_PORT, &g);

    g.GPIO_Pin  = UART_RX_PIN;
    g.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(UART_PORT, &g);

    u.USART_BaudRate            = CFG_UART_BAUDRATE;
    u.USART_WordLength          = USART_WordLength_8b;
    u.USART_StopBits            = USART_StopBits_1;
    u.USART_Parity              = USART_Parity_No;
    u.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    u.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &u);
    USART_Cmd(USART1, ENABLE);
}

void uart_putc(char c)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) { }
    USART_SendData(USART1, (uint16_t)(uint8_t)c);
}

void uart_puts(const char *s)
{
    while (*s) {
        uart_putc(*s++);
    }
}

static void put_u32(uint32_t v)
{
    char buf[10];
    int  n = 0;

    if (v == 0u) {
        uart_putc('0');
        return;
    }
    while (v > 0u) {
        buf[n++] = (char)('0' + (v % 10u));
        v /= 10u;
    }
    while (n-- > 0) {
        uart_putc(buf[n]);
    }
}

static void put_s32(int32_t v)
{
    if (v < 0) {
        uart_putc('-');
        v = -v;
    }
    put_u32((uint32_t)v);
}

static void put_hex(uint32_t v)
{
    static const char h[] = "0123456789ABCDEF";
    char buf[8];
    int  i;

    for (i = 7; i >= 0; i--) {
        buf[i] = h[v & 0xFu];
        v >>= 4;
    }
    for (i = 0; i < 8; i++) {
        uart_putc(buf[i]);
    }
}

void uart_printf(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            uart_putc(*fmt);
            continue;
        }
        fmt++;
        switch (*fmt) {
        case 'd': put_s32((int32_t)va_arg(ap, int));               break;
        case 'u': put_u32((uint32_t)va_arg(ap, unsigned int));     break;
        case 'x': put_hex((uint32_t)va_arg(ap, unsigned int));     break;
        case 'c': uart_putc((char)va_arg(ap, int));                break;
        case 's': uart_puts(va_arg(ap, const char *));             break;
        case '%': uart_putc('%');                                  break;
        default:  uart_putc('%'); uart_putc(*fmt);                 break;
        }
    }
    va_end(ap);
}

/* ================= RX 行接收（非阻塞，运行时调参命令） =================
 * uart_rx_poll() 每个主循环调用一次；收到 CR/LF 结束一行。
 * uart_rx_line() 取走完整一行（含 '\0'），返回 1；无则返回 0。
 */
static char    s_rx_line[CFG_UART_RX_LINE_MAX];
static uint8_t s_rx_len;
static uint8_t s_rx_ready;

void uart_rx_poll(void)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET) {
        char c = (char)(USART_ReceiveData(USART1) & 0xFFu);

        if (c == '\r' || c == '\n') {
            if (s_rx_len > 0u) {
                s_rx_line[s_rx_len] = '\0';
                s_rx_ready = 1u;
                s_rx_len   = 0u;
            }
        } else if (c == '\b' || c == 0x7F) {       /* 退格编辑 */
            if (s_rx_len > 0u) {
                s_rx_len--;
            }
        } else if (s_rx_ready == 0u &&
                   s_rx_len < (uint8_t)(CFG_UART_RX_LINE_MAX - 1u)) {
            s_rx_line[s_rx_len++] = c;
        }
        /* 上一行未取走或行超长时丢弃新字符，不会越界/覆盖 */
    }
}

uint8_t uart_rx_line(char *buf, uint8_t size)
{
    uint8_t i;

    if (!s_rx_ready || size == 0u) {
        return 0u;
    }
    s_rx_ready = 0u;

    for (i = 0u; i < (uint8_t)(size - 1u) && s_rx_line[i] != '\0'; i++) {
        buf[i] = s_rx_line[i];
    }
    buf[i] = '\0';
    return 1u;
}
