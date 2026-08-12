# 基于 CH32V003 的莫尔斯电码二叉树 LED 解析器

一个用 RISC-V 单片机 **CH32V003F4P6** 实现的莫尔斯电码（Morse Code）解析与可视化系统：通过板载按键或外接电键拍发点划，实时在 37 颗 LED 组成的二叉树灯阵上"走树"，并锁定字符经 UART 输出。

## 功能特性

- 自动识别点 `·` / 划 `−`（按拍发时长判决，速度由电位器实时调节 60~300ms）
- 二叉树（堆编号）算法实时解析，支持 **A–Z 26 个字母 + 0–9 10 个数字**（5 层树，37 个节点灯）
- LED 树实时展示走树过程，单点 / 路径两种显示模式（CLEAR 长按切换）
- 字符锁定常亮反馈并 UART 输出；非法码 / 越界全树闪烁报错
- 拍发侧音（蜂鸣器，音量可通过串口调节）
- 全裸机非阻塞状态机，1ms SysTick 时基，无 RTOS、无动态内存

## 仓库结构

```text
.
├─ 莫尔斯二叉树解码器_设计文档_V1.4.md      # 硬件设计规格（画板/BOM/软件契约）
├─ 莫尔斯二叉树解码器_设计文档_V1.3.md      # 上一版硬件设计规格
├─ CH32V003_莫尔斯解析器_软件架构与开发规划_V1.1.md  # 软件架构与开发规划
├─ 查漏补缺与优化方案_V1.4.md              # 原理图审查结论与修正方案
├─ SCH_Schematic1_2026-08-12.pdf           # 原理图
├─ Netlist_Schematic1_2026-08-12.tel       # 网表
└─ firmware/
   └─ Morse_CH32V003/                      # 固件工程（MounRiver Studio 2）
      ├─ User/                             # 全部自研代码（6 层结构）
      │  ├─ app/      应用调度层
      │  ├─ core/     纯莫尔斯核心（morse_core / morse_table，可 PC 单测）
      │  ├─ service/  服务层（led_tree / error_manager / diagnostics）
      │  ├─ driver/   驱动层（shift595 / key / clear / adc / buzzer / uart）
      │  ├─ bsp/      板级层（board / system_time）
      │  └─ config/   配置层（所有魔法数字集中于此）
      ├─ Vendor/                            # WCH 官方外设库与启动文件
      ├─ build.sh / build.bat               # 命令行构建脚本
      └─ test_host/test_morse.c             # 莫尔斯核心的 PC 端单元测试
```

## 快速开始

### 硬件（概要）

| 模块 | 说明 |
|---|---|
| MCU | CH32V003F4P6（TSSOP-20，16K Flash / 2K RAM，5V 直供） |
| LED 驱动 | 74HC595 × 5 级联（40bit，实际用 37 路） |
| 输入 | 板载 KEY + 3.5mm 电键并联（PA1）；CLEAR 功能键（PA2） |
| 调速 | 10kΩ 电位器（PC4 / ADC） |
| 侧音 | 无源蜂鸣器（PD0 / TIM1_CH1N PWM，经 S8050 驱动） |
| 输出 | UART（PD5/PD6，115200 8N1） |

完整引脚分配、BOM、逐网络连接清单见《设计文档 V1.4》§4 / §12 / §16。

### 构建固件

1. 安装 **MounRiver Studio 2** 并记录版本，下载 **CH32V003 EVT** 官方例程包。
2. 按 `firmware/Morse_CH32V003/README.md` 建立工程（Vendor 文件清单见 `Vendor/README.md`）。
3. 命令行构建（使用 MRS2 自带 `riscv-none-embed-gcc`）：

```bash
cd firmware/Morse_CH32V003
bash build.sh normal     # 正式莫尔斯解析固件
bash build.sh bringup    # 板卡 bring-up 自检固件（LED 流水/按键回显/ADC 打印）
```

产物输出到 `build/Morse_CH32V003.hex / .bin`。

### 单元测试（纯 C，PC 端）

莫尔斯核心不依赖硬件，可在 PC 上直接验证：

```bash
gcc -Wall -Wextra \
    -I firmware/Morse_CH32V003/User/core -I firmware/Morse_CH32V003/User/config \
    firmware/Morse_CH32V003/test_host/test_morse.c \
    firmware/Morse_CH32V003/User/core/morse_core.c \
    firmware/Morse_CH32V003/User/core/morse_table.c \
    -o test_morse
./test_morse    # 期望输出 ALL TESTS PASSED
```

## 使用说明

- **拍发**：按下即开始计时，松开按时长判决点划；字符间隔 ≥ 2.5T 锁定字符，单词间隔 ≥ 6T 输出空格。
- **CLEAR**：短按回根清空；长按（>1s）切换单点 / 路径显示模式。
- **串口命令**（115200 8N1）：`V<n>`（n=0~100）设置侧音音量，`V?` 查询，`?` 列出帮助。

## 版本

- v0.3.0：新增串口音量调节；电位器保持专职调速。
- v0.2.0：修复 BUG-1（6T 空格分支 key_down 保护）；消除全部 EVT 核对 TODO。
- v0.1.0：首版完整源码。

## License

暂无（项目处于早期开发阶段）。如需开源请补充许可证文件。
