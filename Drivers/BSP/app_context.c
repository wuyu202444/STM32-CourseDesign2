/*
 * key.c
 */

#include "app_context.h"
#include "cmsis_os.h"     // 使用 osDelay, osMessageQueuePut
#include "rtc_user.h"     // 读写 RTC
#include "ws2812.h"       // 控制灯
#include "buzzer.h"       // 控制蜂鸣器
#include "servo.h"        // 舵机指令定义
#include "stdio.h"        // printf
#include "global.h"

// 【新增】必须引入 FreeRTOS 原生头文件，才能使用 taskENTER_CRITICAL
#include "FreeRTOS.h"
#include "task.h"

#define DEFAULT_SOUND  3

// ================= 外部变量引用 =================
extern osMessageQueueId_t q_ServoCmdHandle;

// ================= 全局变量定义 =================
AppContext_t g_App;

static uint32_t last_key_time[4] = {0};

// ================= 内部辅助函数 (Static) =================

static void Key_Auto_Add(uint8_t *val, uint8_t max, GPIO_TypeDef* port, uint16_t pin, uint8_t key_id)
{
    (*val)++;
    if (*val >= max) *val = 0;

    for(int i=0; i<50; i++)
    {
        osDelay(10);
        if (HAL_GPIO_ReadPin(port, pin) != GPIO_PIN_RESET) {
            last_key_time[key_id] = osKernelGetTickCount();
            return;
        }
    }

    while (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET)
    {
        (*val)++;
        if (*val >= max) *val = 0;
        osDelay(100);
    }
    last_key_time[key_id] = osKernelGetTickCount();
}

// 辅助函数：检测K3是长按还是短按
// 返回值: 0=短按, 1=长按
static uint8_t Check_Long_Press(GPIO_TypeDef* port, uint16_t pin)
{
    uint8_t hold_time = 0;
    // 循环检测，每100ms检测一次，持续1秒
    while (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET) // 假设低电平有效
    {
        osDelay(100);
        hold_time++;
        if (hold_time >= 10) { // 超过1000ms (10 * 100ms)
            return 1; // 长按
        }
    }
    return 0; // 短按
}

static void Handle_Key0(void) {
    // 静态变量记录盒子物理状态：0=合上, 1=打开
    // 必须是 static，这样函数退出后它的值还在
    static uint8_t is_box_open = 0;

    // 1. 如果正在闹钟响，Key0 优先处理（这里假设你没这个需求，如果有请加上）
    if (g_App.is_ringing) return;

    // 2. 正常模式下的逻辑
    if (g_App.current_state == SYS_NORMAL) {

        // 【第一层判断】是否有刷卡授权？
        if (g_App.is_rfid_passed == 1)
        {
            // --- 已授权 ---

            // 【第二层判断】当前盒子是关的还是开的？
            if (is_box_open == 0)
            {
                // === 当前是关的 -> 执行打开 ===
                printf("CMD: Open Box\r\n");
                uint8_t cmd = SERVO_CMD_OPEN;
                osMessageQueuePut(q_ServoCmdHandle, &cmd, 0, 0);

                is_box_open = 1; // 标记为已打开

                // 注意：这里【不】上锁，因为还要等用户按第二次关门
            }
            else
            {
                // === 当前是开的 -> 执行关闭并上锁 ===
                printf("CMD: Close Box & RE-LOCK\r\n");
                uint8_t cmd = SERVO_CMD_CLOSE;
                osMessageQueuePut(q_ServoCmdHandle, &cmd, 0, 0);

                is_box_open = 0; // 标记为已关闭

                // 【关键步骤】流程结束，重新上锁！
                g_App.is_rfid_passed = 0;

                // 提示音：长鸣一声表示上锁
                Buzzer_Send_Cmd(BUZZER_MODE_SINGLE, 1000, DEFAULT_SOUND, 500, 0);
            }
        }
        else
        {
            // --- 未授权 (is_rfid_passed == 0) ---
            printf(">>> [KEY0] Access Denied! Please swipe card. <<<\r\n");

            // 拒绝提示音：急促短叫
            osDelay(150);
            Buzzer_Send_Cmd(BUZZER_MODE_SINGLE, 500, DEFAULT_SOUND, 100, 0);
        }
    }
    else if (g_App.current_state == SYS_SETTING_TIME) {
        Key_Auto_Add(&g_App.edit_hour, 24, KEY0_GPIO_Port, KEY0_Pin, 0);
    }
    // 【新增】闹钟设置：小时+
    else if (g_App.current_state == SYS_SETTING_ALARM) {
        printf("Alarm[%d]: Hour++\r\n", g_App.current_alarm_index);
        Key_Auto_Add(&g_App.alarms[g_App.current_alarm_index].hour, 24, KEY0_GPIO_Port, KEY0_Pin, 0);
    }
}

static void Handle_Key1(void) {
    if (g_App.current_state == SYS_NORMAL) {
        printf("CMD: Silence Buzzer\r\n");

        // 1. 无论什么情况，Key1 按下首先关闭蜂鸣器声音
        osDelay(100);
        Buzzer_Send_Cmd(BUZZER_MODE_OFF, 0, 0, 0, 0);
        Buzzer_Stop();

        // 2. 判断：如果是闹钟正在响 (is_ringing == 1)
        if (g_App.is_ringing == 1)
        {
            printf("CMD: Alarm Cleared (Sound & Light OFF)\r\n");

            // 恢复灯光为正常的绿色呼吸灯
            WS2812_Send_Cmd(RGB_MODE_BREATHING, 0, LED_BRIGHTNESS_BREATH, 0, 0);

            // 清除响铃标志位
            g_App.is_ringing = 0;
        }
        // else: 如果不是闹钟响（is_ringing == 0），比如是高温报警或者单纯测试
        // 代码走到这里已经关了声音，但不会改变灯光状态（红灯/蓝灯继续闪，起到静音但保持警示的作用）
    }
    else if (g_App.current_state == SYS_SETTING_TIME) {
        Key_Auto_Add(&g_App.edit_min, 60, KEY1_GPIO_Port, KEY1_Pin, 1);
    }
    // 闹钟设置：分钟+
    else if (g_App.current_state == SYS_SETTING_ALARM) {
        // 使用指针简化代码，避免重复写很长的数组引用
        // g_App.current_alarm_index 是你在 logic 里定义的或假设有的，如果报错请确认 app_context.h 里有定义
        // 注意：你之前的代码用的是 edit_alarm_idx，请确保变量名一致
        uint8_t idx = g_App.current_alarm_index;

        printf("Alarm[%d]: Min++\r\n", idx);
        Key_Auto_Add(&g_App.alarms[idx].min, 60, KEY1_GPIO_Port, KEY1_Pin, 1);
    }
}

static void Handle_Key2(void) {
    if (g_App.current_state == SYS_NORMAL) {
        g_App.is_bluetooth_on = !g_App.is_bluetooth_on;
        printf("CMD: Bluetooth %s\r\n", g_App.is_bluetooth_on ? "ON" : "OFF");

        if(g_App.is_bluetooth_on)
            WS2812_Send_Cmd(RGB_MODE_BLINK, 0, 0, LED_BRIGHTNESS_DEFAULT, 200);
        else
            WS2812_Send_Cmd(RGB_MODE_BREATHING, 0, LED_BRIGHTNESS_BREATH, 0, 0);
    }
    else if (g_App.current_state == SYS_SETTING_TIME) {
        Key_Auto_Add(&g_App.edit_sec, 60, KEY2_GPIO_Port, KEY2_Pin, 2);
    }
    // 【新增】闹钟设置：秒+
    else if (g_App.current_state == SYS_SETTING_ALARM) {
        printf("Alarm[%d]: Sec++\r\n", g_App.current_alarm_index);
        Key_Auto_Add(&g_App.alarms[g_App.current_alarm_index].sec, 60, KEY2_GPIO_Port, KEY2_Pin, 2);
    }
}

// 【重写】Handle_Key3
static void Handle_Key3(void) {
    // 1. 先判断是长按还是短按
    // 注意：这里会阻塞线程最多1秒，但在Logic任务中通常是可以接受的
    uint8_t is_long_press = Check_Long_Press(KEY3_GPIO_Port, KEY3_Pin);

    // ================= 状态：正常模式 =================
    if (g_App.current_state == SYS_NORMAL)
    {
        if (is_long_press) {
            // [长按] -> 进入闹钟设置模式
            printf("State: Enter Alarm Setting\r\n");
            g_App.current_state = SYS_SETTING_ALARM;
            g_App.current_alarm_index = 0; // 默认从第1个开始

            // 灯光提示：紫色静态
            WS2812_Send_Cmd(RGB_MODE_STATIC, LED_BRIGHTNESS_DEFAULT, 0, LED_BRIGHTNESS_DEFAULT, 0);

            // 等待按键释放，防止松手瞬间误触发
            while(HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin) == GPIO_PIN_RESET) osDelay(10);
        }
        else {
            // [短按] -> 进入系统时间设置 (保持原有逻辑)
            printf("State: Enter Time Setting\r\n");
            taskENTER_CRITICAL();
            RTC_Get_Time(&g_App.edit_hour, &g_App.edit_min, &g_App.edit_sec);
            taskEXIT_CRITICAL();

            g_App.current_state = SYS_SETTING_TIME;
            WS2812_Send_Cmd(RGB_MODE_STATIC, LED_BRIGHTNESS_DEFAULT, LED_BRIGHTNESS_DEFAULT, 0, 0);
        }
    }
    // ================= 状态：闹钟设置模式 =================
    else if (g_App.current_state == SYS_SETTING_ALARM)
    {
        if (is_long_press) {
            // [长按] -> 保存并退出到正常模式
            printf("State: Save Alarms & Exit\r\n");
            // 这里不需要写RTC硬件，因为闹钟数据保存在g_App.alarms里，
            // 之后的逻辑代码会去读取这个数组

            g_App.current_state = SYS_NORMAL;
            WS2812_Send_Cmd(RGB_MODE_BREATHING, 0, LED_BRIGHTNESS_BREATH, 0, 0);

            while(HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin) == GPIO_PIN_RESET) osDelay(10);
        }
        else {
            // [短按] -> 切换下一个闹钟
            g_App.current_alarm_index++;
            if (g_App.current_alarm_index >= MAX_ALARMS) {
                g_App.current_alarm_index = 0;
            }
            printf("Switch to Alarm Index: %d\r\n", g_App.current_alarm_index);
        }
    }
    // ================= 状态：时间设置模式 =================
    else if (g_App.current_state == SYS_SETTING_TIME)
    {
        // 原有逻辑：按Key3 (无论长短) 都保存时间并退出
        printf("State: Save Time & Exit\r\n");
        taskENTER_CRITICAL();
        RTC_Set_Time(g_App.edit_hour, g_App.edit_min, g_App.edit_sec);
        taskEXIT_CRITICAL();

        g_App.current_state = SYS_NORMAL;
        WS2812_Send_Cmd(RGB_MODE_BREATHING, 0, LED_BRIGHTNESS_BREATH, 0, 0);
    }
}

// ================= 对外接口实现 =================

// 初始化函数补充闹钟默认值
void Key_App_Init(void)
{
    g_App.current_state = SYS_NORMAL;
    g_App.is_bluetooth_on = 0;

    // 初始化3个闹钟的默认时间
    for(int i=0; i<MAX_ALARMS; i++) {
        g_App.alarms[i].hour = 23; // 方便调试，可以改成当前时间之后一点点
        g_App.alarms[i].min  = 0;
        g_App.alarms[i].sec  = 0;

        // 【关键修改】必须设为 1，否则 logic 里的 if (g_App.alarms[i].enable) 永远为假
        g_App.alarms[i].enable = 1;
    }
    g_App.current_alarm_index = 0;

    // 【建议】初始化时清除响铃标志
    g_App.is_ringing = 0;

    // 【新增】默认无权限
    g_App.is_rfid_passed = 0;
}



void Key_Process_Event(uint8_t key_id)
{
    uint32_t current_time = osKernelGetTickCount();

    if (key_id >= 4) return;

    if (current_time - last_key_time[key_id] < 50) {
        return;
    }

    last_key_time[key_id] = current_time;

    switch (key_id)
    {
        case 0: Handle_Key0(); break;
        case 1: Handle_Key1(); break;
        case 2: Handle_Key2(); break;
        case 3: Handle_Key3(); break;
        default: break;
    }
}