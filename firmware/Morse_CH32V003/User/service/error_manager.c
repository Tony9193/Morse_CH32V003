/**
 * error_manager.c — 错误反馈统一入口
 *
 * 反馈策略：LED 全树闪 3 次（非阻塞）+ UART 记录。
 * 错误动画不阻塞输入扫描（软件规划 §11.5）：新元素到来时
 * led_tree_show_node() 会自动取消动画，系统直接恢复可用状态。
 */
#include "error_manager.h"
#include "led_tree.h"
#include "uart_debug.h"
#include "system_time.h"

void error_manager_report(error_code_t code)
{
    switch (code) {
    case ERROR_NODE_OVERFLOW:
        DBG("ERR: node overflow (reset to root)\r\n");
        led_tree_error_feedback(now_ms());
        break;
    case ERROR_INVALID_CODE:
        DBG("ERR: invalid morse code\r\n");
        led_tree_error_feedback(now_ms());
        break;
    case ERROR_ADC_ABNORMAL:
        DBG("WARN: adc abnormal\r\n");    /* V1 不做视觉反馈（软件规划 §12.3） */
        break;
    default:
        break;
    }
}
