//
// Created by Administrator on 2025/12/7.
//

/*
 * key.h
 * 负责定义系统状态机、全局上下文以及按键处理接口
 */

#ifndef __APP_CONTEXT_H
#define __APP_CONTEXT_H

#include "main.h"
#include "global.h"

// ================= 配置宏定义 =================


// ================= 数据结构定义 =================

// 1. 系统运行状态枚举
typedef enum {
    SYS_NORMAL = 0,       // 正常模式
    SYS_SETTING_TIME,     // 设置时间模式
    SYS_SETTING_ALARM     // 【新增】设置闹钟模式
} SystemState_t;

// 【新增】闹钟结构体
typedef struct {
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t enable; // 【必须补上这个】0=关, 1=开
} AlarmTime_t;

// 2. 系统全局上下文结构体
typedef struct {
    SystemState_t current_state;
    uint8_t is_bluetooth_on;

    // 设置模式下的临时时间变量 (设置系统时间用)
    uint8_t edit_hour;
    uint8_t edit_min;
    uint8_t edit_sec;

    // 【新增】闹钟相关
    AlarmTime_t alarms[MAX_ALARMS]; // 闹钟数组
    uint8_t is_ringing;      // 0=未响铃, 1=正在响铃
    uint8_t current_alarm_index;    // 当前正在设置第几个闹钟 (0~2)

    // 【新增】RFID 授权标志位 (0=未授权/锁定, 1=已授权/解锁)
    uint8_t is_rfid_passed;
} AppContext_t;

// 3. 声明全局变量 (供其他文件访问，如Task_OLED)
extern AppContext_t g_App;

// ================= 函数声明 =================

/**
 * @brief 初始化 App 上下文 (设置默认状态)
 */
void Key_App_Init(void);

/**
 * @brief 按键事件核心处理函数
 * 包含了消抖、状态机分发、长按连发逻辑
 * @param key_id 按键ID (0~3)
 */
void Key_Process_Event(uint8_t key_id);

#endif /* __KEY_H */