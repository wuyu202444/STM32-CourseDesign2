//
// Created by Administrator on 2025/12/9.
//

#include "rtc_user.h"
#include "rtc.h" // 引用 CubeMX 生成的 rtc.h

// 定义一个“魔术字”，写在备份寄存器里，标记时间是否已设置
#define RTC_BKP_SIGNATURE  0x32F2

/**
 * @brief 设置时间 (Binary 格式)
 */
void RTC_Set_Time(uint8_t h, uint8_t m, uint8_t s)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    // 1. 设置时间结构体
    sTime.Hours = h;
    sTime.Minutes = m;
    sTime.Seconds = s;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;

    // 2. 写入硬件
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    {
        Error_Handler();
    }

    // 3. 必须同时也设置日期，否则有时候 RTC 寄存器锁死不走字
    // 默认设一个假日期，比如 2025年1月1日
    sDate.WeekDay = RTC_WEEKDAY_MONDAY;
    sDate.Month = RTC_MONTH_JANUARY;
    sDate.Date = 1;
    sDate.Year = 25; // 2025

    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
    {
        Error_Handler();
    }

    // 4. 写备份寄存器，标记“我已经设置过时间了”
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, RTC_BKP_SIGNATURE);
}

/**
 * @brief 获取时间 (Binary 格式)
 */
void RTC_Get_Time(uint8_t *h, uint8_t *m, uint8_t *s)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    // 1. 读取时间
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

    // 2. 读取日期 (必须读，哪怕不用！这是硬件解锁步骤)
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    // 3. 赋值输出
    *h = sTime.Hours;
    *m = sTime.Minutes;
    *s = sTime.Seconds;
}

/**
 * @brief 上电初始化检查
 */
void RTC_Check_And_Init(void)
{
    // 读取备份寄存器 DR1
    uint32_t bkp_val = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);

    // 如果读出来的不是我们的签名，说明是第一次上电（或电池没电了）
    if (bkp_val != RTC_BKP_SIGNATURE)
    {
        // 设置默认时间：12:00:00
        RTC_Set_Time(0, 0, 0);
    }
    else
    {
        // 已经设置过了，什么都不做，让时间继续走
    }
}