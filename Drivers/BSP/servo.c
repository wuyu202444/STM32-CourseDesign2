//
// Created by Administrator on 2025/12/8.
//

#include "servo.h"
#include "tim.h" // CubeMX生成的定时器头文件，为了引用 htim3

/**
  * @brief  舵机初始化
  * @note   启动 PWM 输出
  */
void Servo_Init(void)
{
    // 开启 TIM3_CH3 的 PWM 输出
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);

    // 初始化归位到 0度 (关盖)
    Servo_SetAngle(0);
}

/**
  * @brief  设置舵机角度
  * @param  angle: 0 ~ 180 度
  * @note   基于 1MHz 时钟配置:
  *         0度   = 0.5ms = 500
  *         90度  = 1.5ms = 1500
  *         180度 = 2.5ms = 2500
  */
void Servo_SetAngle(uint8_t angle)
{
    uint32_t pulse_value;

    // 限制角度范围，防止烧舵机
    if (angle > 180) angle = 180;

    // 线性映射公式
    // pulse = 500 + (angle * (2500 - 500) / 180)
    pulse_value = 500 + (angle * 2000 / 180);

    // 修改 CCR 寄存器
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, pulse_value);
}