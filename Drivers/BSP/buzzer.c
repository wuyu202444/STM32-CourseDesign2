/*
* buzzer.c
 */
#include "buzzer.h"
#include "tim.h"         // 引用 htim2
#include "cmsis_os2.h"

// 引用外部定义的队列句柄
extern osMessageQueueId_t q_BuzzerDataHandle;

// 设置 PWM 频率和占空比
static void Buzzer_SetPWM(uint16_t frequency, uint8_t volume) {
    if (frequency == 0 || volume == 0) {
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0);
        return;
    }
    // 假设时钟 1MHz (需确认Prescaler设置)
    uint32_t arr_value = (1000000 / frequency) - 1;
    __HAL_TIM_SET_AUTORELOAD(&htim2, arr_value);

    // 限制音量最大100
    if(volume > 100) volume = 100;

    // 计算占空比 (50%最响)
    uint32_t compare_value = (arr_value * volume) / 200;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, compare_value);
}

void Buzzer_Init(void) {
    Buzzer_Stop();
    // 预分频器设置，确保计数频率约为 1MHz
    // 假设 PCLK1 = 100MHz, PSC = 99 -> 1MHz
    __HAL_TIM_SET_PRESCALER(&htim2, 99);
}

void Buzzer_Beep(uint16_t frequency, uint8_t volume) {
    Buzzer_SetPWM(frequency, volume);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
}

void Buzzer_Stop(void) {
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
}

// 发送指令到队列
void Buzzer_Send_Cmd(BuzzerMode_t mode, uint16_t freq, uint8_t vol, uint16_t on_ms, uint16_t off_ms) {
    BuzzerCmd_t msg;
    msg.mode = mode;
    msg.frequency = freq;
    msg.volume = vol;
    msg.duration_on = on_ms;
    msg.duration_off = off_ms;

    if (q_BuzzerDataHandle != NULL) {
        osMessageQueuePut(q_BuzzerDataHandle, &msg, 0, 0);
    }
}