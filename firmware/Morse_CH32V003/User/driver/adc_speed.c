/**
 * adc_speed.c — PC4 单通道软件触发采样，4 次平均，线性映射为 T
 *
 * 已对照 CH32V003 EVT（EXAM/ADC/ADC_DMA）与官方资料逐项确认：
 *   1. ADC 为 10-bit，数据右对齐，满量程 1023（CFG_ADC_FULL_SCALE）。
 *      依据：WCH 官方 GitHub openwch/ch32v003 README "1组10位ADC"；
 *      WCH 官方博客"ADC 数据寄存器为 10 位、精度 VCC/1024"。
 *      （注意：CH32V103/F103 才是 12 位，勿混淆。）
 *   2. PC4 = ADC_Channel_2（EVT 例程注释明确 "ADC channel 2 (PC4)"）
 *   3. 采样时间枚举：ADC_SampleTime_241Cycles（最长档，电位器高阻源更稳）
 *   4. ADC 时钟：RCC_PCLK2_Div8（24MHz/8 = 3MHz，满足上限要求）
 *   5. 校准流程：ADC_Calibration_Vol → ADC_Cmd → Reset/StartCalibration
 * 本文件与 board_config.h 之外不得改动 ADC 相关假设（软件规划 P1-01）。
 */
#include "adc_speed.h"
#include "board_config.h"
#include "app_config.h"
#include "ch32v00x_adc.h"
#include "ch32v00x_gpio.h"
#include "ch32v00x_rcc.h"

static uint16_t s_raw_avg;    /* 平均后原值 */
static uint16_t s_T;          /* 映射后的基准时间 ms */
static uint32_t s_next_ms;    /* 下次采样时刻 */

void adc_speed_init(void)
{
    ADC_InitTypeDef  a;
    GPIO_InitTypeDef g;

    /* PC4 模拟输入（EVT 例程同款配置，引脚宏来自 board_config.h） */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_ADC1, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div8);             /* 24MHz/8 = 3MHz ADC 时钟 */
    g.GPIO_Pin  = SPEED_ADC_PIN;
    g.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOC, &g);

    ADC_DeInit(ADC1);
    a.ADC_Mode               = ADC_Mode_Independent;
    a.ADC_ScanConvMode       = DISABLE;
    a.ADC_ContinuousConvMode = DISABLE;
    a.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;
    a.ADC_DataAlign          = ADC_DataAlign_Right;
    a.ADC_NbrOfChannel       = 1;
    ADC_Init(ADC1, &a);

    ADC_RegularChannelConfig(ADC1, SPEED_ADC_CHANNEL, 1, ADC_SampleTime_241Cycles);
    ADC_Calibration_Vol(ADC1, ADC_CALVOL_50PERCENT);   /* EVT 例程的电压校准步骤 */
    ADC_Cmd(ADC1, ENABLE);

    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1)) { }
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1)) { }

    s_T     = (uint16_t)((CFG_T_MIN_MS + CFG_T_MAX_MS) / 2);
    s_next_ms = 0u;
}

static uint16_t adc_sample_once(void)
{
    volatile uint32_t timeout = 100000u;

    /* BUG 修复：读 RDATAR（ADC_GetConversionValue）不会清 EOC（vendor 库已确认），
     * 若不清，连续采样时后几次会立即读到上一次的陈旧值，导致 CFG_ADC_AVG_N 次
     * 平均退化为"1 次有效 + N-1 次重复"，T 值几乎不随电位器更新。 */
    ADC_ClearFlag(ADC1, ADC_FLAG_EOC);

    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET) {
        if (--timeout == 0u) {
            return 0u;      /* ADC 异常时不卡死整机（返回低值，T 钳位在最小档） */
        }
    }
    return (uint16_t)ADC_GetConversionValue(ADC1);
}

void adc_speed_update(uint32_t now_ms_)
{
    uint32_t sum = 0u;
    uint32_t t;
    uint8_t  i;

    if ((int32_t)(now_ms_ - s_next_ms) < 0) {
        return;             /* 未到采样周期 */
    }
    s_next_ms = now_ms_ + CFG_ADC_UPDATE_MS;

    for (i = 0u; i < CFG_ADC_AVG_N; i++) {
        sum += adc_sample_once();
    }
    s_raw_avg = (uint16_t)(sum / CFG_ADC_AVG_N);

    /* 线性映射：raw / FULL_SCALE * (T_MAX - T_MIN) + T_MIN
     * FULL_SCALE 集中在配置层（软件规划 P1-01） */
    t = (uint32_t)CFG_T_MIN_MS +
        ((uint32_t)s_raw_avg * (uint32_t)(CFG_T_MAX_MS - CFG_T_MIN_MS)) / CFG_ADC_FULL_SCALE;
    if (t < (uint32_t)CFG_T_MIN_MS) { t = CFG_T_MIN_MS; }
    if (t > (uint32_t)CFG_T_MAX_MS) { t = CFG_T_MAX_MS; }
    s_T = (uint16_t)t;
}

uint16_t adc_speed_get_T(void)
{
    return s_T;
}

uint16_t adc_speed_get_raw(void)
{
    return s_raw_avg;
}
