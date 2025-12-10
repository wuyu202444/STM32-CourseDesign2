//
// Created by Administrator on 2025/12/9.
//

#include "ws2812.h"
#include "tim.h"         // 引用 CubeMX 生成的 tim.h 以获取 htim1
#include "cmsis_os2.h"   // 引用 RTOS API 以使用队列功能

// ================= 外部变量引用 =================
// 这个句柄是在 freertos.c 中定义的，这里通过 extern 引用
extern osMessageQueueId_t q_RGBCmdHandle;

// ================= 参数定义 =================
/* * 参数计算 (基于 HCLK=100MHz, ARR=124):
 * 1 Tick = 10ns
 * 周期 Total = 125 * 10ns = 1.25us
 * * 0 Code: High time 0.35us (±150ns) -> 35 ticks -> 设置为 38 (0.38us) 比较稳
 * 1 Code: High time 0.80us (±150ns) -> 80 ticks -> 设置为 80 (0.80us) 比较稳
 */
#define WS_BIT_0   38
#define WS_BIT_1   80

// ================= 全局变量 =================
// 逻辑颜色缓存
RGB_Color_TypeDef RGB_Data[WS2812_NUM_RGBS];

// DMA 发送缓冲区 (物理层)
// 每个灯24位 + 50个周期的 Reset 信号 (保持低电平 > 50us)
#define DMA_BUFFER_SIZE (WS2812_NUM_RGBS * 24 + 50)
uint16_t ws2812_dma_buffer[DMA_BUFFER_SIZE] = {0};

// DMA 传输忙碌标志位 (防重入)
volatile uint8_t ws2812_busy_flag = 0;

// ================= 函数实现 =================

/**
 * @brief  初始化 WS2812 (上电先关灯)
 */
void WS2812_Init(void)
{
    WS2812_SetAll(0, 0, 0);
    WS2812_Show();
}

/**
 * @brief  设置单个灯的颜色 (仅修改缓存，不刷新)
 */
void WS2812_SetColor(uint8_t led_id, uint8_t r, uint8_t g, uint8_t b)
{
    if (led_id >= WS2812_NUM_RGBS) return;
    RGB_Data[led_id].R = r;
    RGB_Data[led_id].G = g;
    RGB_Data[led_id].B = b;
}

/**
 * @brief  设置所有灯为同一颜色 (辅助函数)
 */
void WS2812_SetAll(uint8_t r, uint8_t g, uint8_t b)
{
    for(int i = 0; i < WS2812_NUM_RGBS; i++) {
        WS2812_SetColor(i, r, g, b);
    }
}

/**
 * @brief  将逻辑缓存的数据转换为 PWM 占空比，并启动 DMA 发送
 */
void WS2812_Show(void)
{
    // 如果上一次 DMA 还没发完，放弃本次刷新或等待
    // 这里选择放弃本次，以免阻塞 RTOS 任务
    if (ws2812_busy_flag) return;

    uint32_t buffer_index = 0;

    for (int i = 0; i < WS2812_NUM_RGBS; i++)
    {
        // WS2812B 发送顺序: G -> R -> B (高位在前)
        uint32_t color_val = (RGB_Data[i].G << 16) | (RGB_Data[i].R << 8) | RGB_Data[i].B;

        for (int k = 23; k >= 0; k--)
        {
            // 判断当前位是 1 还是 0，填入对应的 PWM 比较值
            if ((color_val >> k) & 0x01)
            {
                ws2812_dma_buffer[buffer_index] = WS_BIT_1;
            }
            else
            {
                ws2812_dma_buffer[buffer_index] = WS_BIT_0;
            }
            buffer_index++;
        }
    }

    // 填充 Reset 信号 (至少 50us 低电平)
    // 我们的 PWM 周期是 1.25us，50个周期 = 62.5us，足够 Reset
    // 占空比填 0 即为低电平
    for (int i = 0; i < 50; i++)
    {
        ws2812_dma_buffer[buffer_index] = 0;
        buffer_index++;
    }

    // 标记忙碌
    ws2812_busy_flag = 1;

    // 启动 DMA 传输 (TIM1 Channel 4)
    HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_4, (uint32_t *)ws2812_dma_buffer, buffer_index);
}

/**
 * @brief  对外接口：发送灯光控制指令
 * @note   线程安全，非阻塞
 */
void WS2812_Send_Cmd(RGBMode_t mode, uint8_t r, uint8_t g, uint8_t b, uint16_t duration)
{
    RGBCmd_t msg;

    // 1. 封装数据包
    msg.mode = mode;
    msg.R = r;
    msg.G = g;
    msg.B = b;
    msg.duration = duration;

    // 2. 发送到队列
    // Timeout = 0 (如果不成功直接丢弃，不卡死系统)
    // 需要确保 q_RGBCmdHandle 已在 freertos.c 中初始化
    if (q_RGBCmdHandle != NULL) {
        osMessageQueuePut(q_RGBCmdHandle, &msg, 0, 0);
    }
}

// ================= 回调函数 =================

/**
 * @brief  DMA 传输完成回调 (由 HAL 库自动调用)
 */
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    // 判断是否是 TIM1 的 Channel 4
    if (htim->Instance == TIM1 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4)
    {
        // 1. 停止 DMA 输出，防止波形重复
        HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_4);

        // 2. 强制将 CCR 寄存器归零，确保数据线在空闲时保持低电平
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, 0);

        // 3. 清除忙碌标志，允许下一次刷新
        ws2812_busy_flag = 0;
    }
}