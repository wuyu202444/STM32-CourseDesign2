//
// Created by Administrator on 2025/12/7.
//

#ifndef __LM75_H
#define __LM75_H

#include "main.h" // 包含HAL库定义

/*
 * LM75 地址计算 (根据原理图):
 * 基础地址: 1 0 0 1 A2 A1 A0
 * 原理图接地: 1 0 0 1 0  0  0  = 0x48
 * HAL库是8位地址(左移1位): 0x48 << 1 = 0x90
 */
#define LM75_ADDR_WRITE  0x90
#define LM75_ADDR_READ   0x91
#define LM75_REG_TEMP    0x00  // 温度寄存器地址

// 初始化函数，传入CubeMX生成的I2C句柄
void LM75_Init(I2C_HandleTypeDef *hi2c);

// 读取温度函数
float LM75_Read(void);

#endif