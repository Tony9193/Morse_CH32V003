/**
 * diagnostics.h — Bring-up 诊断（软件规划 §6.14 / 硬件文档 §14）
 *
 * 覆盖原 test_firmware 三个独立测试固件的功能：
 *  1. LED D1~D37 流水灯（验证 595 链路 / 位序 / 映射）
 *  2. KEY 按下/松开时长 UART 回显
 *  3. ADC raw / T 打印
 * 由 APP_BUILD_MODE = APP_MODE_BRINGUP 启用，diagnostics_run() 不返回。
 */
#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

void diagnostics_run(void);

#endif /* DIAGNOSTICS_H */
