/**
 * main.c — 入口
 *
 * APP_MODE_BRINGUP：板卡测试模式（流水灯/按键回显/ADC 打印），不返回。
 * APP_MODE_NORMAL ：正式莫尔斯解析固件。
 * 切换位置：User/config/app_config.h
 */
#include "app_config.h"
#include "app_controller.h"

#if APP_BUILD_MODE == APP_MODE_BRINGUP
#include "diagnostics.h"
#endif

int main(void)
{
    app_controller_init();

#if APP_BUILD_MODE == APP_MODE_BRINGUP
    diagnostics_run();      /* 不返回 */
#endif

    for (;;) {
        app_controller_run();
    }
}
