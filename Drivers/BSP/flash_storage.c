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

/**
 * @brief 将当前内存中的闹钟配置写入内部Flash
 * 注意：Flash擦写会暂停CPU取指，必须在临界区进行
 */
void Flash_Save_Settings(void)
{
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t SectorError = 0;
    uint64_t *pData;
    uint32_t addr = FLASH_SAVE_ADDR;

    // 1. 准备数据
    SystemSettings_t settings_to_save;
    settings_to_save.magic = FLASH_MAGIC_NUM;
    // 将当前的全局闹钟数据复制到临时结构体
    memcpy(settings_to_save.alarms, g_App.alarms, sizeof(g_App.alarms));

    // 计算需要写入的 64-bit (8字节) 字的数量
    // STM32F4 标准编程通常按字(32bit)或双字(64bit)操作，HAL库F4通常支持按字/半字/字节
    // 这里我们简单起见，按字(32-bit)写入，所以要把结构体当成uint32数组
    uint32_t len_in_words = sizeof(SystemSettings_t) / 4;
    if (sizeof(SystemSettings_t) % 4 != 0) len_in_words++; // 补齐

    uint32_t *pSource = (uint32_t *)&settings_to_save;

    // ================== 进入临界区 ==================
    // 防止FreeRTOS切换任务，防止中断打断导致Flash操作异常
    taskENTER_CRITICAL();

    // 2. 解锁 Flash
    HAL_FLASH_Unlock();

    // 3. 擦除扇区 (Sector 7)
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3; // 2.7V to 3.6V
    EraseInitStruct.Sector = FLASH_SECTOR_7;              // 指定扇区7
    EraseInitStruct.NbSectors = 1;

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK)
    {
        // 擦除失败处理
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
            printf("Error: Flash Write Failed at offset %d\r\n", i);
            break;
        }
        addr += 4; // 地址后移4字节
    }

    // 5. 锁定 Flash
    HAL_FLASH_Lock();

    // ================== 退出临界区 ==================
    taskEXIT_CRITICAL();

    printf(">> Flash Save Success! Addr: 0x%X, Size: %d bytes\r\n", FLASH_SAVE_ADDR, sizeof(SystemSettings_t));
}

/**
 * @brief 从Flash读取配置
 * 如果Flash是空的（或者魔数不对），则保持默认值
 */
void Flash_Load_Settings(void)
{
    // 直接将Flash地址强转为结构体指针
    SystemSettings_t *pSaved = (SystemSettings_t *)FLASH_SAVE_ADDR;

    // 检查魔数是否正确
    if (pSaved->magic == FLASH_MAGIC_NUM)
    {
        printf(">> Flash Data Found. Loading...\r\n");

        // 只有魔数匹配才覆盖全局变量
        taskENTER_CRITICAL(); // 读取速度快，但也加个临界区保险
        memcpy(g_App.alarms, pSaved->alarms, sizeof(g_App.alarms));
        taskEXIT_CRITICAL();

        // 打印一下读出来的第一个闹钟验证
        printf("   Loaded Alarm[0]: %02d:%02d:%02d\r\n",
               g_App.alarms[0].hour, g_App.alarms[0].min, g_App.alarms[0].sec);
    }
    else
    {
        printf(">> No Valid Flash Data (Magic: 0x%08X). Using Defaults.\r\n", pSaved->magic);
        // 如果没有数据，保持 Key_App_Init 中的默认值，不做任何操作
    }
}