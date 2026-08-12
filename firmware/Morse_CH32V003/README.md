# Morse_CH32V003 固件工程

基于 CH32V003F4P6 的莫尔斯电码二叉树 LED 解析器固件。

- 硬件基线：《莫尔斯二叉树解码器_设计文档_V1.4》（2026-08-12）
- 软件基线：《CH32V003_莫尔斯解析器_软件架构与开发规划_V1.1》
- 架构：裸机 + 1ms SysTick 时基 + 非阻塞状态机，6 层结构（见软件规划 §4）

## 目录结构

```text
Morse_CH32V003/
├─ README.md          本文件
├─ AGENTS.md          AI/人协作硬规则（改代码前必读）
├─ User/              本项目全部自研代码
│  ├─ main.c
│  ├─ ch32v00x_it.c   中断入口（NMI/HardFault；SysTick 在 system_time.c）
│  ├─ app/            Layer 6 应用调度（app_controller）
│  ├─ core/           Layer 5 纯莫尔斯核心（morse_core / morse_table，不碰硬件）
│  ├─ service/        Layer 4 服务（led_tree / error_manager / diagnostics）
│  ├─ driver/         Layer 3 驱动（shift595 / key / clear / adc / buzzer / uart）
│  ├─ bsp/            Layer 2 板级（board / system_time）
│  └─ config/         配置层（board_config.h / app_config.h，所有魔法数字集中在此）
├─ Vendor/            WCH 官方文件放置处（见 Vendor/README.md）
└─ test_firmware/     说明：bring-up 测试已合并为 BRINGUP 构建模式，见下文
```

## 准备工作（一次性）

1. 安装 MounRiver Studio（或 MRS2），记录版本号，不要中途升级。
2. 下载 CH32V003 EVT 官方例程包（wch.cn 下载中心搜 "CH32V003 EVT"）。
3. 准备 WCH-LinkE 下载器。

## 建立工程步骤

1. MounRiver 新建工程，芯片选 `CH32V003F4P6`，模板任选（建立后立即清理）。
2. 把 EVT 中需要的官方文件复制到 `Vendor/`，清单见 `Vendor/README.md`。
3. 删除/不加入工程：EVT 模板自带的 `main.c` 与 `ch32v00x_it.c`（本工程自带）。
4. 把 `User/` 下全部 `.c` 加入工程，头文件包含路径加入：
   `User`、`User/config`、`User/bsp`、`User/driver`、`User/core`、`User/service`、`User/app` 及 `Vendor/` 各层。
5. 编译，要求 0 error。

## 构建模式

在 `User/config/app_config.h` 中切换：

| 宏 | 值 | 用途 |
|---|---|---|
| `APP_BUILD_MODE` | `APP_MODE_BRINGUP` | 板卡 bring-up 测试：LED 流水、按键时长回显、ADC/T 打印（对应软件规划 §23 的⑦⑧⑨与硬件文档 §14 步骤 3~5） |
| `APP_BUILD_MODE` | `APP_MODE_NORMAL` | 正式莫尔斯解析固件 |
| `APP_DEBUG` | 1 / 0 | UART 调试打印开关（Release 置 0，仅输出解码字符） |

说明：软件规划 §5 曾建议 5 个独立 test_firmware 小工程；本工程以一个 BRINGUP 模式覆盖同等功能，避免维护多套工程。

## 串口运行时命令（NORMAL 模式）

电位器专职调速（T 60~300ms），不挪用；音量改走串口，在 115200 8N1 下直接敲命令回车：

| 命令 | 作用 |
|---|---|
| `V<n>`（n=0~100） | 设置侧音音量，回车返回 `VOL=n%` |
| `V?` | 查询当前音量 |
| `?` | 列出命令 |

原理：音量 = TIM1 侧音 PWM 占空比百分比（0=静音，100=满幅）。上电默认值在
`app_config.h` 的 `CFG_VOLUME_DEFAULT_PCT`（当前 50）。CH32V003 无 EEPROM，
掉电后回到默认值；发声中改音量立即生效。

## 烧录与调试接线（硬件文档 §16.6 / 软件规划 §3.4）

H1（4P）：1=+5V，2=GND，3=SWIO，4=NRST，接 WCH-LinkE（跳线选 5V 档）。
H2（2P）：1=UART_TX，2=UART_RX，接 USB 转 TTL，**115200 8N1**。

**供电二选一，禁止双路 5V 并联**：WCH-LinkE 供电时不要同时插 USB-C；USB-C 供电时 LinkE 只连 GND/SWIO/NRST。

## Bring-up 顺序（硬件文档 §14）

1. 不上电目检（方向/极性/焊桥）。
2. 焊 MCU 前上电测 +5V。
3. 烧 BRINGUP 固件：37 颗 LED 按 D1→D37 顺序流水 → 验证 595 链路与位序（bit39 first / bit0 落 QA）。
4. 拍按键看 UART：`KEY UP dur=xxx ms`；RED1 应同步亮灭。
5. 旋转电位器看 `ADC raw=xxx T=xxx ms`，T 应覆盖 60~300ms 且单调。
6. 全部通过后切 `APP_MODE_NORMAL`，拍 `.-.` 应锁定输出 `R`，再测 SOS 与全表。

## 已对照 EVT 核对项（原 TODO(verify)，全部消除）

软件规划 P1-01 / P1-09 / AI Task 06 要求的核对已全部完成（v0.2），结论如下：

1. ADC：10 位，满量程 1023（依据：WCH 官方 GitHub openwch/ch32v003 "1组10位ADC"、
   WCH 官方博客"精度 VCC/1024"；CH32V103/F103 才是 12 位，勿混淆）；PC4 = ADC_Channel_2；
   采样时间 `ADC_SampleTime_241Cycles`；时钟 RCC_PCLK2_Div8 = 3MHz；
   校准流程 `ADC_Calibration_Vol` → `ADC_Cmd` → Reset/StartCalibration（对照 EXAM/ADC/ADC_DMA）。
2. 蜂鸣器：PD0 = TIM1_CH1N 默认复用，PWM 初始化含 BDTR 配置（对照 EVT ComplementaryOutput_DeadTime）。
3. SysTick：CH32V003 的 SysTick 是 WCH 自定义外设，无 `SysTick_Config()`，直接写寄存器
   （CTLR=0x0F，CMP = SystemCoreClock/1000-1）；Handler 用 `WCH-Interrupt-fast`
   （对照 EVT EXAM/SYSTICK/SYSTICK_Interrupt）。
4. USART1 默认 TX/RX = PD5/PD6（对照 EVT USART 例程，与硬件文档一致）。

后续如需复核，修改只允许发生在 `driver/` 与 `bsp/`；`core/` 不得引入任何硬件头文件。

## 命令行构建（不依赖 IDE）

工具链使用 MounRiver Studio 2 自带的 riscv-none-embed-gcc 8.2.0：

```bat
build.bat            REM NORMAL 正式固件
build.bat bringup    REM BRINGUP 板卡自检固件
```

产物输出到 `build/Morse_CH32V003.hex / .bin`（release/ 内有已构建的成品副本）。
NORMAL 版资源占用：Flash 约 7.2KB/16KB，RAM 静态 476B + 256B 栈 / 2KB。

## 版本

- v0.3.0（2026-08-12）：新增串口音量调节（`V<n>`/`V?`，PWM 占空比 0~100，默认 50）；
  电位器保持专职调速；UART 增加非阻塞行接收（支持退格编辑）。
- v0.2.0（2026-08-12）：修复 BUG-1（6T 空格分支补 `key_down` 保护，含回归测试 10）；
  SysTick/ADC/蜂鸣器/USART 全部 TODO(verify) 对照 EVT 消除；修正 ADC 注释矛盾
  （确认为 10 位/满量程 1023）；新增 build.sh/build.bat 命令行构建，首次产出可烧录成品。
- v0.1.0（2026-08-12）：首版完整源码，未上板验证。
