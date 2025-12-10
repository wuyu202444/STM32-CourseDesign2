/*
* flash_storage.h
 */
#ifndef FLASH_STORAGE_H
#define FLASH_STORAGE_H

#include "main.h"
#include "app_context.h" // 包含 AlarmTime_t 和 MAX_ALARMS 定义

// 定义Flash存储的起始地址 (Sector 7 Start for STM32F411RE)
// 如果你的程序非常大超过了384KB，请检查Map文件，否则Sector 7是安全的
#define FLASH_SAVE_ADDR  0x08060000

// 定义一个魔数，用于判断Flash里是否已经存过有效数据
#define FLASH_MAGIC_NUM  0xA5A55A5A

// 定义我们要存的结构体
typedef struct {
    uint32_t magic;                 // 标志位
    AlarmTime_t alarms[MAX_ALARMS]; // 闹钟数组
    // 如果后续有其他需要保存的配置（如音量、灵敏度），加在这里
} SystemSettings_t;

// 函数声明
void Flash_Save_Settings(void);
void Flash_Load_Settings(void);

#endif