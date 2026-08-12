/**
 * ch32v00x_it.h — 中断入口声明（本工程自带，替代 EVT 模板中的同名文件）
 *
 * EVT 模板版本会 #include "debug.h"（官方调试串口），本工程不引入 debug.c，
 * 故提供此精简版。ch32v00x_conf.h 会包含本文件。
 */
#ifndef __CH32V00x_IT_H
#define __CH32V00x_IT_H

#include "ch32v00x.h"

void NMI_Handler(void);
void HardFault_Handler(void);

#endif /* __CH32V00x_IT_H */
