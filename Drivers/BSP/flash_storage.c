/*
 * flash_storage.c
 */
#include "flash_storage.h"
#include "string.h"
#include "stdio.h"
#include "FreeRTOS.h"
#include "task.h"

// 引入全局变量以便读取和写入
extern AppContext_t g_App;
extern CRC_HandleTypeDef hcrc; // 【新增】引用 main.c 定义的 CRC 句柄

/**
 * @brief 计算结构体数据的 CRC 值
 * @param pSettings 指向配置结构体的指针
 * @return 计算出的 32位 CRC 值
 */
static uint32_t Calculate_Settings_CRC(SystemSettings_t *pSettings)
{
    // 计算长度：(结构体总大小 - CRC字段本身大小) / 4
    // 因为 HAL_CRC_Calculate 按字(32-bit)计算，所以长度要除以4
    uint32_t length_in_words = (sizeof(SystemSettings_t) - sizeof(uint32_t)) / 4;

    // 使用硬件 CRC 计算前面数据的校验值
    // 这里的 (uint32_t *)pSettings 将结构体看作一个数组
    return HAL_CRC_Calculate(&hcrc, (uint32_t *)pSettings, length_in_words);
}

/**
 * @brief 保存数据到 Flash (带 CRC 校验)
 */
void Flash_Save_Settings(void)
{
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t SectorError = 0;
    uint32_t addr = FLASH_SAVE_ADDR;

    // 1. 准备数据
    SystemSettings_t settings_to_save;
    settings_to_save.magic = FLASH_MAGIC_NUM;
    memcpy(settings_to_save.alarms, g_App.alarms, sizeof(g_App.alarms));

    // 【新增】计算并填充 CRC
    settings_to_save.crc_val = Calculate_Settings_CRC(&settings_to_save);

    // 准备写入参数
    uint32_t len_in_words = sizeof(SystemSettings_t) / 4;
    if (sizeof(SystemSettings_t) % 4 != 0) len_in_words++;

    uint32_t *pSource = (uint32_t *)&settings_to_save;

    // ================== 进入临界区 ==================
    taskENTER_CRITICAL();

    // 2. 解锁 Flash
    HAL_FLASH_Unlock();

    // 3. 擦除扇区
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    EraseInitStruct.Sector = FLASH_SECTOR_7;
    EraseInitStruct.NbSectors = 1;

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK)
    {
        printf("Error: Flash Erase Failed!\r\n");
        HAL_FLASH_Lock();
        taskEXIT_CRITICAL();
        return;
    }

    // 4. 写入数据
    for (uint32_t i = 0; i < len_in_words; i++)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, pSource[i]) != HAL_OK)
        {
            printf("Error: Flash Write Failed!\r\n");
            break;
        }
        addr += 4;
    }

    // 5. 锁定 Flash
    HAL_FLASH_Lock();

    taskEXIT_CRITICAL();
    // ================== 退出临界区 ==================

    printf(">> Flash Saved. CRC: 0x%08X\r\n", settings_to_save.crc_val);
}

/**
 * @brief 从 Flash 读取数据 (带 CRC 校验)
 */
void Flash_Load_Settings(void)
{
    SystemSettings_t *pSaved = (SystemSettings_t *)FLASH_SAVE_ADDR;
    uint32_t calc_crc = 0;

    // 1. 检查 Magic Number
    if (pSaved->magic == FLASH_MAGIC_NUM)
    {
        // 2. 【新增】计算 Flash 中数据的 CRC
        // 我们不直接计算 pSaved->crc_val，只计算前面的数据，看结果是否等于 pSaved->crc_val
        calc_crc = Calculate_Settings_CRC(pSaved);

        if (calc_crc == pSaved->crc_val)
        {
            printf(">> Flash Integrity OK (CRC: 0x%08X). Loading...\r\n", calc_crc);

            taskENTER_CRITICAL();
            memcpy(g_App.alarms, pSaved->alarms, sizeof(g_App.alarms));
            taskEXIT_CRITICAL();

            printf("   Alarm[0]: %02d:%02d:%02d\r\n",
                   g_App.alarms[0].hour, g_App.alarms[0].min, g_App.alarms[0].sec);
        }
        else
        {
            // CRC 校验失败（数据损坏）
            printf(">> ERROR: Flash CRC Mismatch! Read: 0x%08X, Calc: 0x%08X\r\n",
                   pSaved->crc_val, calc_crc);
            printf(">> Using Default Settings.\r\n");
            // 此时不执行加载，保持默认值
        }
    }
    else
    {
        printf(">> No Flash Data / First Run (Magic: 0x%08X). Using Defaults.\r\n", pSaved->magic);
    }
}