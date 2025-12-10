//
// Created by Administrator on 2025/12/9.
//

#ifndef __WS2812_H
#define __WS2812_H

#include "main.h"

// ================= 配置区 =================
// 根据你的原理图，药箱上有2个灯珠，如果更多请修改此处
#define WS2812_NUM_RGBS  2

// ================= 数据结构 =================

// 1. 颜色结构体
typedef struct {
    uint8_t R;
    uint8_t G;
    uint8_t B;
} RGB_Color_TypeDef;

// 2. 灯效模式枚举
typedef enum {
    RGB_MODE_OFF = 0,    // 关灯
    RGB_MODE_STATIC,     // 常亮
    RGB_MODE_BLINK,      // 闪烁 (报警)
    RGB_MODE_BREATHING   // 呼吸 (待机)
} RGBMode_t;

// 3. 内部指令包结构体 (RTOS队列交互用)
typedef struct {
    RGBMode_t mode;      // 模式
    uint8_t R;           // 红
    uint8_t G;           // 绿
    uint8_t B;           // 蓝
    uint16_t duration;   // 持续时间 或 闪烁间隔(ms)
} RGBCmd_t;

// ================= 函数声明 =================

// --- 基础驱动函数 (一般由 Task_RGB 内部调用) ---
void WS2812_Init(void);
void WS2812_Show(void);
void WS2812_SetColor(uint8_t led_id, uint8_t r, uint8_t g, uint8_t b);
void WS2812_SetAll(uint8_t r, uint8_t g, uint8_t b);

// --- 外部API (供 Logic 任务或其他任务调用) ---
// 线程安全：直接把指令丢进队列，不会阻塞调用者
void WS2812_Send_Cmd(RGBMode_t mode, uint8_t r, uint8_t g, uint8_t b, uint16_t duration);

#endif /* __WS2812_H */