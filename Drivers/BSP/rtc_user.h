//
// Created by Administrator on 2025/12/9.
//

#ifndef __RTC_USER_H
#define __RTC_USER_H

#include "main.h"

// 相关的 RTC 句柄，CubeMX 生成在 rtc.c 里
extern RTC_HandleTypeDef hrtc;

// ================= 函数声明 =================

/**
 * @brief 检查是否是首次上电。
 * 如果是首次，设置默认时间；如果不是，保持当前走时。
 */
void RTC_Check_And_Init(void);

/**
 * @brief 设置时间
 * @param h 时 (0-23)
 * @param m 分 (0-59)
 * @param s 秒 (0-59)
 */
void RTC_Set_Time(uint8_t h, uint8_t m, uint8_t s);

/**
 * @brief 获取当前时间
 * @param h [输出] 时
 * @param m [输出] 分
 * @param s [输出] 秒
 */
void RTC_Get_Time(uint8_t *h, uint8_t *m, uint8_t *s);

#endif