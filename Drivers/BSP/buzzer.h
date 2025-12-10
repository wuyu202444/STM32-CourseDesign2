/*
* buzzer.h
 */
#ifndef __BUZZER_H
#define __BUZZER_H

#include "main.h"
#include "cmsis_os2.h"

// ================= 数据结构 =================

// 1. 工作模式枚举
typedef enum {
    BUZZER_MODE_OFF = 0,    // 停止/静音
    BUZZER_MODE_SINGLE,     // 单次响声 (滴)
    BUZZER_MODE_LOOP        // 循环响声 (滴-滴-滴...)
} BuzzerMode_t;

// 2. 消息结构体 (队列传输用)
typedef struct {
    BuzzerMode_t mode;      // 模式
    uint16_t frequency;     // 频率 (Hz)
    uint8_t  volume;        // 音量 (0-100)
    uint16_t duration_on;   // 响的时间 (ms)
    uint16_t duration_off;  // 停的时间 (ms, 仅LOOP模式有效)
} BuzzerCmd_t;

// ================= 函数声明 =================

void Buzzer_Init(void);
void Buzzer_Beep(uint16_t frequency, uint8_t volume);
void Buzzer_Stop(void);

/**
 * @brief 发送指令 (线程安全)
 * @param mode: 模式 (SINGLE/LOOP/OFF)
 * @param freq: 频率
 * @param vol: 音量
 * @param on_ms: 响多久
 * @param off_ms: 停多久 (循环间隔)
 */
void Buzzer_Send_Cmd(BuzzerMode_t mode, uint16_t freq, uint8_t vol, uint16_t on_ms, uint16_t off_ms);

#endif //__BUZZER_H