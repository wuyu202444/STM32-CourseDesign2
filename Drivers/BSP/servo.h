//
// Created by Administrator on 2025/12/8.
//
#ifndef __SERVO_H
#define __SERVO_H

#include "main.h" // 包含 HAL 库和引脚定义

// 定义舵机控制指令 (用于队列传输)
#define SERVO_CMD_CLOSE  0  // 关盖 (0度)
#define SERVO_CMD_OPEN   1  // 开盖 (90度或180度)

// 函数声明
void Servo_Init(void);
void Servo_SetAngle(uint8_t angle); // 设置角度 0-180

#endif