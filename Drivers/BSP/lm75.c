//
// Created by Administrator on 2025/12/7.
//

#include "lm75.h"

// 静态变量保存I2C句柄，避免每次调用都传参
static I2C_HandleTypeDef *ph_lm75_i2c = NULL;

/**
 * @brief  初始化LM75驱动
 * @param  hi2c: 指向I2C1的句柄 (通常是 &hi2c1)
 */
void LM75_Init(I2C_HandleTypeDef *hi2c) {
    ph_lm75_i2c = hi2c;
}

/**
 * @brief  读取LM75温度
 * @return 浮点型温度值 (摄氏度)
 */
float LM75_Read(void) {
    uint8_t raw_data[2];
    int16_t temp_int;

    // 保护：如果还没初始化，返回错误值 (如 -999.0)
    if (ph_lm75_i2c == NULL) return -999.0f;

    /*
     * 使用 HAL_I2C_Mem_Read 读取寄存器
     * 参数1: I2C句柄
     * 参数2: 设备地址 (0x90)
     * 参数3: 寄存器地址 (0x00 温度寄存器)
     * 参数4: 寄存器地址长度 (1字节)
     * 参数5: 数据缓冲区
     * 参数6: 读取长度 (2字节，高8位+低8位)
     * 参数7: 超时时间 (100ms)
     */
    if (HAL_I2C_Mem_Read(ph_lm75_i2c, LM75_ADDR_WRITE, LM75_REG_TEMP,
                         I2C_MEMADD_SIZE_8BIT, raw_data, 2, 100) != HAL_OK) {
        return -999.0f; // 读取失败
                         }

    /* 数据处理逻辑 (LM75标准 11-bit 分辨率)
     * Byte 0: 高8位 (整数部分 + 符号位)
     * Byte 1: 低8位 (高3位有效，代表 0.5, 0.25, 0.125)
     *
     * 拼接成16位整数: (High << 8) | Low
     * 因为数据是左对齐的 (11位有效)，所以需要右移5位
     * 分辨率是 0.125°C
     */

    // 组合成16位有符号整数
    temp_int = (int16_t)((raw_data[0] << 8) | raw_data[1]);

    // 右移5位，并乘以精度 0.125
    // 注意：利用 int16_t 的符号扩展特性处理负温
    return (float)(temp_int >> 5) * 0.125f;
}