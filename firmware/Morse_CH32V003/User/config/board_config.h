/**
 * board_config.h — 板级引脚契约（唯一引脚分配依据）
 *
 * 依据：硬件设计文档 V1.4 §4 / §16.2（已逐脚核对）。
 * 其他模块禁止硬编码端口/引脚，一律引用本文件宏。
 */
#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "ch32v00x.h"

/* ---------------- KEY 输入：PA1（电键 + 板载按键并联，低有效） ---------------- */
/* 板上已有 R38 1k 硬上拉，MCU 再开内部上拉（双保险，抗长线干扰） */
#define KEY_PORT        GPIOA
#define KEY_PIN         GPIO_Pin_1
#define KEY_RCC_CLK     RCC_APB2Periph_GPIOA

/* ---------------- CLEAR 功能键：PA2（低有效，内部上拉即可） ---------------- */
#define CLEAR_PORT      GPIOA
#define CLEAR_PIN       GPIO_Pin_2

/* ---------------- 74HC595 ×5：PC0~PC3 ---------------- */
#define SR595_PORT      GPIOC
#define SR595_RCC_CLK   RCC_APB2Periph_GPIOC
#define PIN_595_SER     GPIO_Pin_0      /* 数据（链首 U2-14） */
#define PIN_595_SRCLK   GPIO_Pin_1      /* 移位时钟，五片并联 */
#define PIN_595_RCLK    GPIO_Pin_2      /* 锁存，五片并联 */
#define PIN_595_OE      GPIO_Pin_3      /* 输出使能，低有效！ */

/* ---------------- 速度电位器：PC4 = ADC 输入 ---------------- */
/* 已对照 EVT EXAM/ADC/ADC_DMA 确认：PC4 = ADC 通道 2（TV-2 消除）。
 * 模拟输入 GPIO 初始化在 adc_speed.c（GPIO_Mode_AIN）。 */
#define SPEED_ADC_PIN       GPIO_Pin_4
#define SPEED_ADC_CHANNEL   ADC_Channel_2

/* ---------------- 蜂鸣器：PD0（TIM1_CH1N PWM，经 S8050 驱动） ---------------- */
/* 已对照 EVT EXAM/TIM/ComplementaryOutput_DeadTime 确认：
 * PD0 = TIM1_CH1N 默认复用（无需 remap），GPIO 需配 GPIO_Mode_AF_PP（TV-6 消除）。 */
#define BUZZER_PORT     GPIOD
#define BUZZER_PIN      GPIO_Pin_0
#define BUZZER_RCC_CLK  RCC_APB2Periph_GPIOD

/* ---------------- UART1：PD5=TX / PD6=RX（CH32V003 USART1 默认脚，硬件文档已核对） ---------------- */
#define UART_PORT       GPIOD
#define UART_RCC_CLK    RCC_APB2Periph_GPIOD
#define UART_TX_PIN     GPIO_Pin_5
#define UART_RX_PIN     GPIO_Pin_6

/* ---------------- 禁区（任何初始化不得触碰） ---------------- */
/* PD1 = SWIO（烧录生命线）；PD7 = NRST（复位脚） */
/* PC5/PC6/PC7、PD2/PD3/PD4 为备用脚（J4 扩展排针），V1 固件保持复位默认态不初始化 */

#endif /* BOARD_CONFIG_H */
