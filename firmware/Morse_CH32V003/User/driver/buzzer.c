/**
 * buzzer.c — PD0 / TIM1_CH1N PWM 侧音
 *
 * 已对照 EVT EXAM/TIM/ComplementaryOutput_DeadTime 确认（TV-6 消除）：
 * PD0 = TIM1_CH1N 默认复用（无需 remap），GPIO 配 AF_PP；
 * 高级定时器输出需 TIM_BDTRConfig + TIM_CtrlPWMOutputs 使能主输出。
 * 音量：PWM 占空比百分比（0~100，0=静音），buzzer_set_volume() 设置，
 * 默认值 CFG_VOLUME_DEFAULT_PCT；串口 V 命令可调（掉电不保存）。
 * 若短期调不通，可临时把 PD0 改回普通 GPIO 在主循环翻转方波，
 * 蜂鸣器不在核心功能路径上（软件规划 §12.4）。
 */
#include "buzzer.h"
#include "board_config.h"
#include "app_config.h"
#include "ch32v00x_gpio.h"
#include "ch32v00x_rcc.h"
#include "ch32v00x_tim.h"

static uint16_t s_duty_full;  /* 100% 占空比对应的比较值（= arr+1） */
static uint8_t  s_volume;     /* 音量百分比 0~100 */
static uint8_t  s_sounding;   /* 1 = 正在发声 */

static uint16_t duty_for_volume(void)
{
    return (uint16_t)(((uint32_t)s_duty_full * (uint32_t)s_volume) / 100u);
}

void buzzer_init(void)
{
    GPIO_InitTypeDef        g;
    TIM_TimeBaseInitTypeDef tb;
    TIM_OCInitTypeDef       oc;
    TIM_BDTRInitTypeDef     bdtr;
    uint32_t                psc;
    uint32_t                arr;

    RCC_APB2PeriphClockCmd(BUZZER_RCC_CLK | RCC_APB2Periph_TIM1, ENABLE);

    g.GPIO_Pin   = BUZZER_PIN;
    g.GPIO_Mode  = GPIO_Mode_AF_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BUZZER_PORT, &g);

    /* 时基：1MHz 计数 → CFG_TONE_FREQ_HZ。用 SystemCoreClock 计算，不写死 24MHz */
    psc = SystemCoreClock / 1000000ul - 1ul;
    arr = 1000000ul / (uint32_t)CFG_TONE_FREQ_HZ - 1ul;

    TIM_TimeBaseStructInit(&tb);
    tb.TIM_Prescaler         = (uint16_t)psc;
    tb.TIM_CounterMode       = TIM_CounterMode_Up;
    tb.TIM_Period            = (uint16_t)arr;
    tb.TIM_ClockDivision     = TIM_CKD_DIV1;
    tb.TIM_RepetitionCounter = 0u;
    TIM_TimeBaseInit(TIM1, &tb);

    TIM_OCStructInit(&oc);
    oc.TIM_OCMode       = TIM_OCMode_PWM1;
    oc.TIM_OutputState  = TIM_OutputState_Disable;
    oc.TIM_OutputNState = TIM_OutputNState_Enable;   /* 使用互补通道 CH1N */
    oc.TIM_Pulse        = 0u;                        /* 初始占空比 0 = 静音 */
    oc.TIM_OCPolarity   = TIM_OCPolarity_High;
    /* BUG 修复：PD0 是 TIM1_CH1N 互补（反相）输出。OCNPolarity_High 下
     * CH1N = !OC1REF：CCR=0（buzzer_off 意图静音）反而输出高 → S8050 导通、
     * 蜂鸣器常响；音量越高越静。改 Low 使 CH1N 与 OC1REF 同相，占空比才与
     * 响度成正比（0=静音，100=满幅）。 */
    oc.TIM_OCNPolarity  = TIM_OCNPolarity_Low;
    oc.TIM_OCIdleState  = TIM_OCIdleState_Reset;
    oc.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
    TIM_OC1Init(TIM1, &oc);
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);

    /* BDTR（EVT 例程同款）：不锁、无死区需要、禁止刹车、自动输出 */
    TIM_BDTRStructInit(&bdtr);
    bdtr.TIM_OSSIState       = TIM_OSSIState_Disable;
    bdtr.TIM_OSSRState       = TIM_OSSRState_Disable;
    bdtr.TIM_LOCKLevel       = TIM_LOCKLevel_OFF;
    bdtr.TIM_DeadTime        = 0u;
    bdtr.TIM_Break           = TIM_Break_Disable;
    bdtr.TIM_BreakPolarity   = TIM_BreakPolarity_High;
    bdtr.TIM_AutomaticOutput = TIM_AutomaticOutput_Enable;
    TIM_BDTRConfig(TIM1, &bdtr);

    TIM_CtrlPWMOutputs(TIM1, ENABLE);    /* 高级定时器必须使能主输出 */
    TIM_ARRPreloadConfig(TIM1, ENABLE);
    TIM_Cmd(TIM1, ENABLE);

    s_duty_full = (uint16_t)(arr + 1ul);
    s_volume    = CFG_VOLUME_DEFAULT_PCT;
}

void buzzer_on(void)
{
    s_sounding = 1u;
    TIM_SetCompare1(TIM1, duty_for_volume());
}

void buzzer_off(void)
{
    s_sounding = 0u;
    TIM_SetCompare1(TIM1, 0u);
}

void buzzer_set_volume(uint8_t percent)
{
    if (percent > 100u) {
        percent = 100u;
    }
    s_volume = percent;

    /* 正在发声时立即生效 */
    if (s_sounding) {
        TIM_SetCompare1(TIM1, duty_for_volume());
    }
}

uint8_t buzzer_get_volume(void)
{
    return s_volume;
}
