/**
 * error_manager.h — 统一错误类型与反馈（软件规划 §6.15）
 */
#ifndef ERROR_MANAGER_H
#define ERROR_MANAGER_H

typedef enum {
    ERROR_NODE_OVERFLOW,    /* node > 63 */
    ERROR_INVALID_CODE,     /* 锁定到无定义字符的节点 */
    ERROR_ADC_ABNORMAL      /* 预留：V1 只记录不报错（电位器端点是合法位置） */
} error_code_t;

void error_manager_report(error_code_t code);

#endif /* ERROR_MANAGER_H */
