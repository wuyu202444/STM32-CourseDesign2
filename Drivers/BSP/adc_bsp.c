//
// Created by Administrator on 2025/12/7.
//

#include "adc_bsp.h"

/* 引入CubeMX生成的ADC句柄，定义在 main.c 中 */
extern ADC_HandleTypeDef hadc1;

/*
 * 定义DMA缓存变量
 * volatile 关键字非常重要！告诉编译器这个变量会被硬件(DMA)修改，不要优化读取操作。
 */
volatile uint16_t g_ADC_Value = 0;

/**
  * @brief  启动ADC和DMA (在任务开始前调用一次即可)
  */
void BSP_ADC_Init(void)
{
    // 开启ADC校准（可选，提高精度，部分型号需要）
    // HAL_ADCEx_Calibration_Start(&hadc1);

    // 启动ADC，并告诉DMA把数据搬运到 g_ADC_Value 变量中
    // 长度为1，因为我们只采集一个通道
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&g_ADC_Value, 1);
}

/**
  * @brief  读取当前电位器数值
  * @return 12位ADC值 (0~4095)
  * @note   由于配置了DMA Circular模式，这里不需要等待转换，直接读内存即可
  */
uint16_t ADC_Read(void)
{
    return g_ADC_Value;
}