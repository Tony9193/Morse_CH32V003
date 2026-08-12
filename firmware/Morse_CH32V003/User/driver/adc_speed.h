/**
 * adc_speed.h — 电位器 ADC → 基准时间 T（软件规划 §6.6 / §12.2）
 */
#ifndef ADC_SPEED_H
#define ADC_SPEED_H

#include <stdint.h>

void     adc_speed_init(void);
/* 主循环每圈调用；内部按 CFG_ADC_UPDATE_MS 自行节流，不会每圈都采样 */
void     adc_speed_update(uint32_t now_ms);
uint16_t adc_speed_get_T(void);     /* 当前基准时间 T（ms），已钳位在 T_MIN~T_MAX */
uint16_t adc_speed_get_raw(void);   /* 最近一次平均后的 ADC 原值（诊断用） */

#endif /* ADC_SPEED_H */
