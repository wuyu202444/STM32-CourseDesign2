//
// Created by Administrator on 2025/12/10.
//

/* vtx316.c */
#include "vtx316.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

static UART_HandleTypeDef *vtx_uart_handle;

// ... (之前的 VTX316_Init 和 VTX316_SendFrame 代码保持不变) ...

// ... (确保 VTX316_SendFrame 在这里) ...

void VTX316_Init(UART_HandleTypeDef *huart)
{
    vtx_uart_handle = huart;
}

// 之前的内部发送函数保持不变
static void VTX316_SendFrame(char *text)
{
    if (vtx_uart_handle == NULL) return;
    uint8_t frame[256];
    uint16_t text_len = strlen(text);
    uint16_t data_len = 1 + 1 + text_len;

    if (text_len > 200) return;

    frame[0] = 0xFD;
    frame[1] = (data_len >> 8) & 0xFF;
    frame[2] = data_len & 0xFF;
    frame[3] = VTX_CMD_SPEAK;
    frame[4] = CURRENT_ENCODING;
    memcpy(&frame[5], text, text_len);
    HAL_UART_Transmit(vtx_uart_handle, frame, 5 + text_len, 100);
}

void VTX316_Speak(char *str)
{
    VTX316_SendFrame(str);
}

void VTX316_Printf(char *fmt, ...)
{
    char buf[200];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    VTX316_SendFrame(buf);
}

/**
 * @brief  设置语音参数 (上电设置一次即可)
 * @param  volume: 音量 [0-10], 10最大
 * @param  speed:  语速 [0-10], 5标准
 * @param  role:   发音人 (见头文件宏定义, 如 VTX_ROLE_FEMALE_1)
 */
void VTX316_SetStyle(uint8_t volume, uint8_t speed, uint8_t role)
{
    char cmd_buf[32];

    // 限制范围
    if(volume > 10) volume = 10;
    if(speed > 10)  speed = 10;

    // 拼接控制指令，例如 "[v10][s5][m3]"
    // [v*]: 音量
    // [s*]: 语速
    // [m*]: 发音人
    sprintf(cmd_buf, "[v%d][s%d][m%d]", volume, speed, role);

    VTX316_SendFrame(cmd_buf);
}