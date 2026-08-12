# AGENTS.md — Morse_CH32V003 工程硬规则

任何人或 AI 修改本工程代码前必须遵守以下规则（来源：软件架构与开发规划 V1.1 §20.1）：

1. 不修改 `Vendor/` 目录（WCH 官方文件原样保留）。
2. `PD1` 永远保留给 SWIO：任何 GPIO 初始化、引脚表、自动初始化列表禁止包含 PD1；`PD7/NRST` 同样禁止作 IO。
3. 不使用动态内存（malloc/free）。
4. 不引入 RTOS。
5. 业务代码禁止阻塞式 Delay；所有动画/反馈必须是非阻塞状态机，不得阻塞 KEY 输入扫描。
6. `core/morse_core.c`、`core/morse_table.c` 禁止访问 GPIO、禁止包含 WCH 外设头文件（保持纯 C，可在 PC 上单测）。
7. `NODE_TO_LED` 是唯一的 node→LED 映射，禁止写 `LED = node - 1`（数字节点不连续）。
8. 74HC595 移位顺序必须 `bit39 first / bit0 last`，且 bit0 落在 595#1 的 QA；改位序前先在实板跑流水灯验证。
9. 任何 ADC 满量程、通道号、时钟假设必须引用 CH32V003 官方资料，并集中在配置层的 `CFG_ADC_*` 宏；勿把 4095 散落进业务代码。
10. 修改后必须保持 MounRiver 工程可编译（0 error）；所有魔法数字集中在 `User/config/`。

## 附加约定

- 依赖方向单向：app → (morse_core / led_tree / drivers) → bsp → vendor；禁止反向依赖与模块循环依赖。
- 中断里只做 `tick++` 级别的工作；禁止在 ISR 中 printf、刷 595、跑莫尔斯逻辑。
- 一个字符进行中使用冻结的 `T_char`，不跟随电位器实时变化（软件规划 §9.4）。
- 单词空格每个长间隔只允许输出一次（`word_space_emitted` 闩锁）。
- CLEAR 长按只产生一次 LONG 事件，松手后不得补发 SHORT。
