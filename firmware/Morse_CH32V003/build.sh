#!/bin/bash
# build.sh — 命令行构建 Morse_CH32V003 固件（MounRiver Studio 2 自带工具链）
# 用法：bash build.sh [normal|bringup]
set -e
cd "$(dirname "$0")"

TC="C:/MounRiver/MounRiver_Studio2/resources/app/resources/win32/components/WCH/Toolchain/RISC-V Embedded GCC/bin"
CC="$TC/riscv-none-embed-gcc.exe"
OBJCOPY="$TC/riscv-none-embed-objcopy.exe"
SIZE="$TC/riscv-none-embed-size.exe"

MODE=${1:-normal}
OUT=build
OBJ=$OUT/obj
mkdir -p "$OBJ"

FLAGS="-march=rv32ec -mabi=ilp32e -msmall-data-limit=8 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wall"
INC="-IUser -IUser/config -IUser/bsp -IUser/driver -IUser/core -IUser/service -IUser/app -IVendor -IVendor/peripheral"

if [ "$MODE" = "bringup" ]; then
  FLAGS="$FLAGS -DAPP_BUILD_MODE=APP_MODE_BRINGUP"
  echo "== BRINGUP 构建（板卡自检模式）=="
else
  echo "== NORMAL 构建（正式固件）=="
fi

SRCS="
User/main.c
User/ch32v00x_it.c
User/bsp/board.c
User/bsp/system_time.c
User/driver/shift595.c
User/driver/key_input.c
User/driver/clear_input.c
User/driver/adc_speed.c
User/driver/buzzer.c
User/driver/uart_debug.c
User/core/morse_core.c
User/core/morse_table.c
User/service/led_tree.c
User/service/error_manager.c
User/service/diagnostics.c
User/app/app_controller.c
Vendor/system_ch32v00x.c
Vendor/core_riscv.c
Vendor/peripheral/ch32v00x_gpio.c
Vendor/peripheral/ch32v00x_rcc.c
Vendor/peripheral/ch32v00x_usart.c
Vendor/peripheral/ch32v00x_adc.c
Vendor/peripheral/ch32v00x_tim.c
Vendor/peripheral/ch32v00x_misc.c
"

OBJS=""
for s in $SRCS; do
  o="$OBJ/$(echo "$s" | sed 's|[/.]|_|g').o"
  "$CC" $FLAGS $INC -c "$s" -o "$o"
  OBJS="$OBJS $o"
done
"$CC" $FLAGS $INC -c Vendor/startup_ch32v00x.S -o "$OBJ/startup.o"
OBJS="$OBJS $OBJ/startup.o"

"$CC" $FLAGS -T Vendor/Link.ld -nostartfiles -Xlinker --gc-sections \
  -Wl,-Map=$OUT/firmware.map --specs=nano.specs $OBJS -o $OUT/firmware.elf

"$OBJCOPY" -O ihex   $OUT/firmware.elf $OUT/Morse_CH32V003.hex
"$OBJCOPY" -O binary $OUT/firmware.elf $OUT/Morse_CH32V003.bin
"$SIZE" $OUT/firmware.elf
echo "== 完成：$OUT/Morse_CH32V003.hex / .bin =="
