/**
 * shift595.h — 74HC595 最底层输出驱动（软件规划 §6.7）
 *
 * 本模块只认 40bit 显示位图，不懂字符/node/闪烁语义。
 * 软件契约（硬件文档 §8.2，必须与实板流水灯验证一致）：
 *   bit39 先发，bit0 最后发 → bit0 落在 595#1 的 QA → bit i 点亮 LED i。
 */
#ifndef SHIFT595_H
#define SHIFT595_H

#include <stdint.h>

void shift595_write(uint64_t bits);   /* 移位 40bit + 锁存（bit39 first） */
void shift595_enable(uint8_t on);     /* OE 控制：1 = 显示使能（OE 拉低），0 = 熄灭 */
void shift595_init(void);             /* 安全上电序列：禁输出→清零→锁存→使能 */

#endif /* SHIFT595_H */
