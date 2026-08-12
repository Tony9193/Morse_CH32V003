/**
 * app_controller.h — 应用调度层（软件规划 §6.16 / Layer 6）
 *
 * 它是"调度者"，不是"什么都塞的大 main.c"：
 * 负责初始化顺序（软件规划 §13）与主循环中各模块的轮询和事件分发。
 */
#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

void app_controller_init(void);
void app_controller_run(void);   /* 主循环体，每圈调用一次（非阻塞） */

#endif /* APP_CONTROLLER_H */
