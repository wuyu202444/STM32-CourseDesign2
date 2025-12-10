//
// Created by Administrator on 2025/12/10.
//

/* vtx316.h */
#ifndef __VTX316_H
#define __VTX316_H

#include "main.h"
#include "usart.h"

// 编码格式
#define VTX_ENCODING_GBK   0x01
#define VTX_ENCODING_UTF8  0x05
#define CURRENT_ENCODING   VTX_ENCODING_UTF8

// 命令字
#define VTX_CMD_SPEAK      0x01

// --- 新增：发音人选择 (根据VTX316手册常用定义) ---
#define VTX_ROLE_FEMALE_1  3   // 只要甜美女声 (默认)
#define VTX_ROLE_MALE_1    51  // 浑厚男声
#define VTX_ROLE_MALE_2    52  // 情感男声
#define VTX_ROLE_CHILD     54  // 童声
#define VTX_ROLE_DONALD    4   // 唐老鸭 (特效)

// --- 新增：函数声明 ---
void VTX316_Init(UART_HandleTypeDef *huart);
void VTX316_Speak(char *str);
void VTX316_Printf(char *fmt, ...);

// 新增设置函数：音量(0-10), 语速(0-10), 发音人(宏定义)
void VTX316_SetStyle(uint8_t volume, uint8_t speed, uint8_t role);

#endif /* __VTX316_H */