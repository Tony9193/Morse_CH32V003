/**
 * buzzer.h — 蜂鸣器侧音驱动（软件规划 §6.9 / §12.4）
 *
 * 接口只有 tone_on / tone_off，无阻塞 Delay。
 * 音量 = PWM 占空比百分比（0~100，0 为静音），串口可调，掉电不保存。
 * 蜂鸣器为可选模块，出问题时允许整体禁用，不阻塞核心开发。
 */
#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>

void    buzzer_init(void);
void    buzzer_on(void);
void    buzzer_off(void);
void    buzzer_set_volume(uint8_t percent);   /* 0~100，超出钳位；发声中立即生效 */
uint8_t buzzer_get_volume(void);

#endif /* BUZZER_H */
