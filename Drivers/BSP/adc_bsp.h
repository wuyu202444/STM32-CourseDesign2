//
// Created by Administrator on 2025/12/7.
//

#ifndef __ADC_BSP_H__
#define __ADC_BSP_H__

#include "main.h" // 包含HAL库定义

// 初始化并启动ADC DMA采集
void BSP_ADC_Init(void);

// 获取当前的ADC值 (0 - 4095)
uint16_t ADC_Read(void);

#endif