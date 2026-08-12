/**
 * app_config.h — 全局可调参数（所有魔法数字集中于此，软件规划原则 E）
 */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* ================= 构建模式 ================= */
#define APP_MODE_NORMAL     0   /* 正式莫尔斯解析固件 */
#define APP_MODE_BRINGUP    1   /* 板卡测试：LED 流水 / 按键时长回显 / ADC 打印 */

/* 切换构建模式：改这里的默认值，或编译时传 -DAPP_BUILD_MODE=APP_MODE_BRINGUP */
#ifndef APP_BUILD_MODE
#define APP_BUILD_MODE      APP_MODE_NORMAL
#endif

/* 调试打印开关：1 = UART 输出事件细节；Release 置 0，只保留解码字符输出 */
#define APP_DEBUG           1

/* ================= 按键 ================= */
#define CFG_KEY_DEBOUNCE_MS     15      /* KEY 消抖（硬件文档 §7.1） */
#define CFG_CLEAR_DEBOUNCE_MS   15      /* CLEAR 消抖 */
#define CFG_CLEAR_LONG_MS       1000    /* CLEAR 长按门限（>1s 切换显示模式） */

/* ================= 莫尔斯时间 ================= */
#define CFG_T_MIN_MS            60      /* 电位器映射下限（约 20 WPM） */
#define CFG_T_MAX_MS            300     /* 电位器映射上限（约 5 WPM） */

/* 判决门限（×10 整数，避免浮点；实际门限 = T * x10 / 10） */
#define CFG_DOT_DASH_x10        20      /* 按下 < 2.0T → 点，否则划 */
#define CFG_CHAR_GAP_x10        25      /* 松开 ≥ 2.5T → 锁定字符 */
#define CFG_WORD_GAP_x10        60      /* 松开 ≥ 6.0T → 输出单词空格 */

/* ================= ADC ================= */
#define CFG_ADC_UPDATE_MS       100     /* 电位器采样周期（软件规划：50~100ms） */
#define CFG_ADC_AVG_N           4       /* 简单平均次数 */
/* 已确认（沁恒官方资料 + EVT）：CH32V003 ADC 为 10 位，转换值 0~1023，
 * 精度 = VCC/1024。勿再假设 12bit/4095。 */
#define CFG_ADC_FULL_SCALE      1023

/* ================= LED 反馈 ================= */
#define CFG_LOCK_STYLE_HOLD     0       /* 锁定反馈：常亮保持 */
#define CFG_LOCK_STYLE_BLINK    1       /* 锁定反馈：闪烁 */
#define CFG_LOCK_FEEDBACK_STYLE CFG_LOCK_STYLE_HOLD
#define CFG_LOCK_HOLD_MS        1500    /* 常亮保持时长（硬件文档 §8.5：1.5s） */
#define CFG_LOCK_BLINK_TIMES    3
#define CFG_LOCK_BLINK_HALF_MS  150     /* 闪烁半周期 */
#define CFG_ERR_BLINK_TIMES     3       /* 错误：全树闪 3 次 */
#define CFG_ERR_BLINK_HALF_MS   120

/* ================= UART ================= */
#define CFG_UART_BAUDRATE       115200  /* 8N1 */
#define CFG_UART_RX_LINE_MAX    16      /* 串口命令行缓冲区（含结尾 '\0'） */

/* ================= 蜂鸣器 ================= */
#define CFG_TONE_FREQ_HZ        2000    /* 侧音频率（电磁蜂鸣器随意驱动即可发声） */
#define CFG_VOLUME_DEFAULT_PCT  50      /* 上电默认音量（占空比 0~100，0=静音）；
                                         * 串口 V 命令可调，CH32V003 无 EEPROM，掉电不保存 */

#endif /* APP_CONFIG_H */
