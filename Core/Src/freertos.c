/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "adc_bsp.h"
#include "lm75.h"
#include "i2c.h"
#include "oled.h"
#include "BSP_LED_SEG.h"
#include "buzzer.h"
#include "app_context.h"
#include "rc522.h"
#include "servo.h"
#include "string.h"
#include "ws2812.h"
#include "rtc_user.h"
#include "global.h"
#include "vtx316.h"
#include "iwdg.h"
#include "app_context.h"
#include "flash_storage.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

//RGB各个模式的灯光亮度在key.h里

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
/* 定义一个结构体，这就是我们要传输的“包裹” */
typedef struct {
  float temperature;  // 温度
  uint16_t adc_value; // 电位器值
  uint8_t key_status; // 按键状态
} SensorMsg_t;
// 【新增】定义一个全局变量，用于数码管显示温度
float g_DisplayTemp = 0.0f;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Task_Sensor */
osThreadId_t Task_SensorHandle;
const osThreadAttr_t Task_Sensor_attributes = {
  .name = "Task_Sensor",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Task_OLED */
osThreadId_t Task_OLEDHandle;
const osThreadAttr_t Task_OLED_attributes = {
  .name = "Task_OLED",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Task_SEG */
osThreadId_t Task_SEGHandle;
const osThreadAttr_t Task_SEG_attributes = {
  .name = "Task_SEG",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Task_Logic */
osThreadId_t Task_LogicHandle;
const osThreadAttr_t Task_Logic_attributes = {
  .name = "Task_Logic",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Task_Buzzer */
osThreadId_t Task_BuzzerHandle;
const osThreadAttr_t Task_Buzzer_attributes = {
  .name = "Task_Buzzer",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Task_Servo */
osThreadId_t Task_ServoHandle;
const osThreadAttr_t Task_Servo_attributes = {
  .name = "Task_Servo",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for Task_RGB */
osThreadId_t Task_RGBHandle;
const osThreadAttr_t Task_RGB_attributes = {
  .name = "Task_RGB",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for q_SensorData */
osMessageQueueId_t q_SensorDataHandle;
const osMessageQueueAttr_t q_SensorData_attributes = {
  .name = "q_SensorData"
};
/* Definitions for q_BuzzerData */
osMessageQueueId_t q_BuzzerDataHandle;
const osMessageQueueAttr_t q_BuzzerData_attributes = {
  .name = "q_BuzzerData"
};
/* Definitions for q_ServoCmd */
osMessageQueueId_t q_ServoCmdHandle;
const osMessageQueueAttr_t q_ServoCmd_attributes = {
  .name = "q_ServoCmd"
};
/* Definitions for q_RGBCmd */
osMessageQueueId_t q_RGBCmdHandle;
const osMessageQueueAttr_t q_RGBCmd_attributes = {
  .name = "q_RGBCmd"
};
/* Definitions for q_Keypad */
osMessageQueueId_t q_KeypadHandle;
const osMessageQueueAttr_t q_Keypad_attributes = {
  .name = "q_Keypad"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartTask_Sensor(void *argument);
void StartTask_OLED(void *argument);
void StartTask_SEG(void *argument);
void StartTask_Logic(void *argument);
void StartTask_Buzzer(void *argument);
void StartTask_Servo(void *argument);
void StartTask_RGB(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of q_SensorData */
  q_SensorDataHandle = osMessageQueueNew (5, 16, &q_SensorData_attributes);

  /* creation of q_BuzzerData */
  q_BuzzerDataHandle = osMessageQueueNew (5, 12, &q_BuzzerData_attributes);

  /* creation of q_ServoCmd */
  q_ServoCmdHandle = osMessageQueueNew (3, sizeof(uint8_t), &q_ServoCmd_attributes);

  /* creation of q_RGBCmd */
  q_RGBCmdHandle = osMessageQueueNew (4, 8, &q_RGBCmd_attributes);

  /* creation of q_Keypad */
  q_KeypadHandle = osMessageQueueNew (4, 1, &q_Keypad_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of Task_Sensor */
  Task_SensorHandle = osThreadNew(StartTask_Sensor, NULL, &Task_Sensor_attributes);

  /* creation of Task_OLED */
  Task_OLEDHandle = osThreadNew(StartTask_OLED, NULL, &Task_OLED_attributes);

  /* creation of Task_SEG */
  Task_SEGHandle = osThreadNew(StartTask_SEG, NULL, &Task_SEG_attributes);

  /* creation of Task_Logic */
  Task_LogicHandle = osThreadNew(StartTask_Logic, NULL, &Task_Logic_attributes);

  /* creation of Task_Buzzer */
  Task_BuzzerHandle = osThreadNew(StartTask_Buzzer, NULL, &Task_Buzzer_attributes);

  /* creation of Task_Servo */
  Task_ServoHandle = osThreadNew(StartTask_Servo, NULL, &Task_Servo_attributes);

  /* creation of Task_RGB */
  Task_RGBHandle = osThreadNew(StartTask_RGB, NULL, &Task_RGB_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */

  // 1. 硬件初始化
  VTX316_Init(&huart1);
  Key_App_Init(); // 这里会先加载默认值 (例如 23:00:00)

  // 2. 尝试从Flash加载用户设置
  // 必须放在 Key_App_Init 之后，以便覆盖默认值
  Flash_Load_Settings();

  // 3. 开机提示音和灯光
  Buzzer_Send_Cmd(BUZZER_MODE_SINGLE,523, START_SOUND, 100, 0);
  Buzzer_Send_Cmd(BUZZER_MODE_SINGLE,587, START_SOUND, 100, 0);
  Buzzer_Send_Cmd(BUZZER_MODE_SINGLE,659, START_SOUND, 100, 0);
  osDelay(500);

  WS2812_Send_Cmd(RGB_MODE_BREATHING, 0, LED_BRIGHTNESS_BREATH, 0, 0);

  VTX316_SetStyle(1, 5, VTX_ROLE_FEMALE_1);
  osDelay(100);
  VTX316_Speak("开机");

  printf("\r\n================================\r\n");
  printf("   Smart Medicine Box System    \r\n");
  printf("================================\r\n");

  /* Infinite loop */
  for(;;)
  {
    osDelay(1000);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTask_Sensor */
/**
* @brief Function implementing the Task_Sensor thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask_Sensor */
void StartTask_Sensor(void *argument)
{
  /* USER CODE BEGIN StartTask_Sensor */
  BSP_ADC_Init();
  LM75_Init(&hi2c1);
  RC522_Init();

  uint8_t time_count = 0;

  // 【新增】定义合法的卡号 (61-BE-D0-17)
  // 注意：RC522读出的卡号通常是16进制，请确保顺序正确
  uint8_t Authorized_UID[4] = {0x61, 0xBE, 0xD0, 0x17};

  /* Infinite loop */
  for(;;)
  {
    // ==========================================
    // 第一部分：高频任务 (RFID 检测)
    // ==========================================
    uint8_t status;
    uint8_t str[MAXRLEN];

    // 【修复1】必须定义为 5 字节！防止内存溢出导致 A5 乱码
    uint8_t card_id[5];

    // 【修复2】每次清零，防止显示历史残留数据
    memset(card_id, 0, 5);

    status = PcdRequest(PICC_REQALL, str);
    if (status == MI_OK)
    {
      // 这里的 PcdAnticoll 会写满 5 个字节 (4 UID + 1 BCC)
      status = PcdAnticoll(card_id);

      if (status == MI_OK)
      {
        printf(">>> [RFID] UID: %02X-%02X-%02X-%02X <<<\r\n",
               card_id[0], card_id[1], card_id[2], card_id[3]);

        // 1. 刷卡提示音
        Buzzer_Send_Cmd(BUZZER_MODE_SINGLE, 2000, CARD_SOUND, 100, 0);
        osDelay(100);

        // 2. 比对卡号 (只比对前4位)
        if (card_id[0] == Authorized_UID[0] &&
            card_id[1] == Authorized_UID[1] &&
            card_id[2] == Authorized_UID[2] &&
            card_id[3] == Authorized_UID[3])
        {
          printf(">>> [RFID] Correct! Permission Granted. <<<\r\n");
          VTX316_Speak("读卡成功");

          // 【关键】只给权限，不开箱。开箱由 Key0 决定
          g_App.is_rfid_passed = 1;

          // 绿灯闪烁提示成功
          WS2812_Send_Cmd(RGB_MODE_BLINK, 0, 30, 0, 500);
          osDelay(500);
          WS2812_Send_Cmd(RGB_MODE_BREATHING, 0, LED_BRIGHTNESS_BREATH, 0, 0);
        }
        else
        {
          printf(">>> [RFID] Wrong Card! <<<\r\n");
          // 错卡不撤销权限，只报错 (防止误触)
          // 红灯闪烁提示失败
          WS2812_Send_Cmd(RGB_MODE_BLINK, 30, 0, 0, 500);
          osDelay(500);
          WS2812_Send_Cmd(RGB_MODE_BREATHING, 0, LED_BRIGHTNESS_BREATH, 0, 0);
          VTX316_Speak("错误");
        }
      }
      osDelay(500); // 读卡防抖
    }

    // ==========================================
    // 第二部分：低频任务 (温度/ADC)
    // ==========================================
    time_count++;
    if (time_count >= 10) // 1000ms
    {
      time_count = 0;

      SensorMsg_t msg;
      msg.temperature = LM75_Read();
      msg.adc_value = ADC_Read();

      // 【调试】打印采集到的传感器数据
      // printf("[SENSOR] Temp: %.2f C, ADC: %d\r\n", msg.temperature, msg.adc_value);

      osMessageQueuePut(q_SensorDataHandle, &msg, 0, 0);
    }

    osDelay(100);
  }
  /* USER CODE END StartTask_Sensor */
}

/* USER CODE BEGIN Header_StartTask_OLED */
/**
* @brief Function implementing the Task_OLED thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask_OLED */
void StartTask_OLED(void *argument)
{
  /* USER CODE BEGIN StartTask_OLED */
  osDelay(50);
  OLED_Init();
  OLED_Clear();

  char str_buf[20];
  uint8_t h, m, s;

  /* Infinite loop */
  for(;;)
  {
    if (g_App.current_state == SYS_NORMAL)
    {
        RTC_Get_Time(&h, &m, &s);
        OLED_ShowString(0, 1, "Box: Monitor    ", 16, 1);
        sprintf(str_buf, "[ %02d:%02d:%02d ]", h, m, s);
        OLED_ShowString(16, 16, (uint8_t *)str_buf, 16, 1);

        // if(g_App.is_bluetooth_on)
        //     OLED_ShowString(0, 32, "BT: ON          ", 12, 1);
        // else
        //     OLED_ShowString(0, 32, "BT: OFF         ", 12, 1);

        OLED_ShowString(0, 42, "S-K3:Set TIM3   ", 12, 1); // 提示用户单机进入
        OLED_ShowString(0, 52, "L-K3:Set Alarm  ", 12, 1); // 提示用户长按进入
    }
    else if (g_App.current_state == SYS_SETTING_TIME)
    {
        OLED_ShowString(0, 0, ">> SET TIME <<  ", 16, 1);
        sprintf(str_buf, "[ %02d:%02d:%02d ]", g_App.edit_hour, g_App.edit_min, g_App.edit_sec);
        OLED_ShowString(16, 16, (uint8_t *)str_buf, 16, 1);
        OLED_ShowString(0, 32, "Hr+ Mn+ Sc+ (K3)", 12, 1);
        OLED_ShowString(0, 48, "K0  K1  K2  SAVE", 12, 1);
    }
    // 【新增】闹钟设置界面
    else if (g_App.current_state == SYS_SETTING_ALARM)
    {
      // 获取当前正在编辑的闹钟数据
      uint8_t idx = g_App.current_alarm_index;
      AlarmTime_t *pAlarm = &g_App.alarms[idx];

      // 显示标题：Alarm 1/3, Alarm 2/3...
      sprintf(str_buf, ">> SET ALARM %d <<", idx + 1);
      OLED_ShowString(0, 0, (uint8_t *)str_buf, 16, 1);

      // 显示当前闹钟时间
      sprintf(str_buf, "[ %02d:%02d:%02d ]", pAlarm->hour, pAlarm->min, pAlarm->sec);
      OLED_ShowString(16, 16, (uint8_t *)str_buf, 16, 1);

      // 显示操作提示
      OLED_ShowString(0, 32, "K0/1/2: H/M/S+  ", 12, 1);
      OLED_ShowString(0, 48, "S:Next L:Exit   ", 12, 1); // 短按切换，长按退出
    }

    OLED_Refresh();
    osDelay(30);
  }
  /* USER CODE END StartTask_OLED */
}

/* USER CODE BEGIN Header_StartTask_SEG */
/**
* @brief Function implementing the Task_SEG thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask_SEG */
void StartTask_SEG(void *argument)
{
  /* USER CODE BEGIN StartTask_SEG */
  int temp_integer = 0;
  /* Infinite loop */
  for(;;)
  {
    // 1. 获取温度的整数部分
    temp_integer = (int)g_DisplayTemp;

    // 2. 限制显示范围 (例如 0-99)，防止数码管乱码
    if(temp_integer > 99) temp_integer = 99;
    if(temp_integer < 0)  temp_integer = 0;

    // 3. 显示
    // 假设 ShowNum 的第一个参数是数值，第二个参数是填充/格式(根据你之前的代码填了2)
    BSP_LEDSEG_ShowNum(temp_integer / 10, temp_integer % 10);

    osDelay(5); // 保持刷新频率
  }
  /* USER CODE END StartTask_SEG */
}

/* USER CODE BEGIN Header_StartTask_Logic */
/**
* @brief Function implementing the Task_Logic thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask_Logic */
void StartTask_Logic(void *argument)
{
  /* USER CODE BEGIN StartTask_Logic */
  // 1. 初始化检查：如果掉电过，重置为 12:00:00
  RTC_Check_And_Init();

  SensorMsg_t received_msg;
  osStatus_t status;

  // 【修改】删除原来的固定闹钟变量 alarm_h/m/s
  // uint8_t alarm_h = 12; ... (已删除)

  uint8_t curr_h, curr_m, curr_s;
  uint8_t last_s = 255;
  uint8_t key_id;

  // 报警状态标志位 (0=正常, 1=正在报警)
  uint8_t is_temp_alarm = 0;
  uint8_t is_drug_alarm = 0;

  /* Infinite loop */
  for(;;)
  {
    // ==========================================
    // 动作：喂狗 (Refresh Watchdog)
    // ==========================================
    // 告诉看门狗：“我还活着，不要重启我！”
    HAL_IWDG_Refresh(&hiwdg);

    // ========================================================
    // 1. RTC 闹钟逻辑 (多闹钟遍历)
    // ========================================================
    RTC_Get_Time(&curr_h, &curr_m, &curr_s);

    // 每一秒只检测一次，避免一秒内多次触发
    if (curr_s != last_s)
    {
      // 【调试代码】每秒打印一次当前时间和第一个闹钟的状态
      // printf("Time:%02d:%02d:%02d | Alarm0:%02d:%02d:%02d [En:%d]\r\n",
      //        curr_h, curr_m, curr_s,
      //        g_App.alarms[0].hour, g_App.alarms[0].min, g_App.alarms[0].sec,
      //        g_App.alarms[0].enable);

      last_s = curr_s;
      if (g_App.current_state == SYS_NORMAL)
      {
        for(int i = 0; i < 3; i++) // 假设 MAX_ALARMS 为 3
        {
          // 检查：开启 && 时间匹配
          if (g_App.alarms[i].enable &&  // 记得加上 .enable 判断
              curr_h == g_App.alarms[i].hour &&
              curr_m == g_App.alarms[i].min  &&
              curr_s == g_App.alarms[i].sec)
          {
            printf(">>> [LOGIC] ALARM [%d] TRIGGERED! <<<\r\n", i+1);
            VTX316_Speak("请吃药");

            // 【关键修改】设置响铃标志位，告诉系统现在闹钟在响
            g_App.is_ringing = 1;

            // 闹钟音效
            Buzzer_Send_Cmd(BUZZER_MODE_LOOP, 1000, CLOCK_SOUND,  200, 300);
            // 灯光闪烁
            WS2812_Send_Cmd(RGB_MODE_BLINK, 32, 14, 0, 300);
          }
        }
      }
    }

    // ========================================================
    // 2. 传感器数据处理 (保持不变)
    // ========================================================
    status = osMessageQueueGet(q_SensorDataHandle, &received_msg, NULL, 0);

    if (status == osOK)
    {
      g_DisplayTemp = received_msg.temperature;

      // --------------------------------------------------------
      // A. 温度报警逻辑
      // --------------------------------------------------------
      if (received_msg.temperature >= TEMP_LIMIT)
      {
        if (is_temp_alarm == 0)
        {
          printf(">>> [LOGIC] ALERT: Temp High! %.1f C\r\n", received_msg.temperature);
          Buzzer_Send_Cmd(BUZZER_MODE_LOOP, 2000, ALARM_SOUND, 100, 100);
          WS2812_Send_Cmd(RGB_MODE_BLINK, 100, 0, 0, 200);
          is_temp_alarm = 1;
          VTX316_Speak("温度过高");
        }
      }
      else // 温度正常
      {
        if (is_temp_alarm == 1)
        {
          is_temp_alarm = 0;
          // 【关键修改】增加 && g_App.is_ringing == 0
          // 只有当药量正常，且【闹钟没在响】的时候，才关声音
          if (is_drug_alarm == 0 && g_App.is_ringing == 0) {
            Buzzer_Send_Cmd(BUZZER_MODE_OFF, 0, 0, 0, 0);
            WS2812_Send_Cmd(RGB_MODE_BREATHING, 0, LED_BRIGHTNESS_BREATH, 0, 0);
          }
          else if (is_drug_alarm == 1){
            // 恢复为药量报警状态
            Buzzer_Send_Cmd(BUZZER_MODE_LOOP, 500, ALARM_SOUND, 500, 500);
            WS2812_Send_Cmd(RGB_MODE_BLINK, 0, 0, 100, 500);
          }
        }
      }

      // --------------------------------------------------------
      // B. 缺药报警逻辑
      // --------------------------------------------------------
      if (received_msg.adc_value < VOL_LIMIT)
      {
        if (is_drug_alarm == 0)
        {
          printf(">>> [LOGIC] ALERT: Low Drug Level! (ADC: %d)\r\n", received_msg.adc_value);
          // 如果没有高温报警，才开启缺药报警（高温优先级更高）
          if (is_temp_alarm == 0) {
            Buzzer_Send_Cmd(BUZZER_MODE_LOOP, 500, ALARM_SOUND, 500, 500);
            WS2812_Send_Cmd(RGB_MODE_BLINK, 0, 0, 100, 500);
            VTX316_Speak("药量低");
          }
          is_drug_alarm = 1;
        }
      }
      else // 药量正常
      {
        if (is_drug_alarm == 1)
        {
          is_drug_alarm = 0;
          // 【关键修改】增加 && g_App.is_ringing == 0
          // 只有当温度正常，且【闹钟没在响】的时候，才关声音
          if (is_temp_alarm == 0 && g_App.is_ringing == 0) {
            Buzzer_Send_Cmd(BUZZER_MODE_OFF, 0, 0, 0, 0);
            WS2812_Send_Cmd(RGB_MODE_BREATHING, 0, LED_BRIGHTNESS_BREATH, 0, 0);
          }
          else if (is_temp_alarm == 1) {
            // 恢复回高温报警
            Buzzer_Send_Cmd(BUZZER_MODE_LOOP, 2000, ALARM_SOUND, 100, 100);
            WS2812_Send_Cmd(RGB_MODE_BLINK, 100, 0, 0, 200);
          }
        }
      }
    }

    // ========================================================
    // 3. 按键处理
    // ========================================================
    if (osMessageQueueGet(q_KeypadHandle, &key_id, NULL, 0) == osOK)
    {
      // 这里的逻辑主要是处理按键事件，具体执行在 key.c 中
      // 但 key.c 中已经写了 Handle_Key1 -> Buzzer_OFF，
      // 所以如果闹钟响了，用户按 Key1 就会停止闹钟，完美闭环。
      Key_Process_Event(key_id);
    }

    osDelay(10);
  }
  /* USER CODE END StartTask_Logic */
}

/* USER CODE BEGIN Header_StartTask_Buzzer */
/**
* @brief Function implementing the Task_Buzzer thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask_Buzzer */
void StartTask_Buzzer(void *argument)
{
  /* USER CODE BEGIN StartTask_Buzzer */
  Buzzer_Init();

  BuzzerCmd_t current_cmd = {BUZZER_MODE_OFF, 0, 0, 0, 0};
  BuzzerCmd_t new_cmd;
  osStatus_t status;

  /* Infinite loop */
  for(;;)
  {
    // 1. 获取指令
    // 如果是 LOOP 模式，只等待 10ms，以便循环执行
    // 如果是 OFF 模式，永久等待(WaitForever)，不耗 CPU
    uint32_t wait_time = (current_cmd.mode == BUZZER_MODE_LOOP) ? 10 : osWaitForever;

    status = osMessageQueueGet(q_BuzzerDataHandle, &new_cmd, NULL, wait_time);

    if (status == osOK) {
      current_cmd = new_cmd; // 有新指令，覆盖旧状态
    }

    // 2. 执行状态机
    switch (current_cmd.mode)
    {
    case BUZZER_MODE_SINGLE:
      // [单次模式] 响 -> 延时 -> 停 -> 自动切回OFF
      Buzzer_Beep(current_cmd.frequency, current_cmd.volume);
      osDelay(current_cmd.duration_on);
      Buzzer_Stop();
      current_cmd.mode = BUZZER_MODE_OFF; // 执行完就自行结束
      break;

    case BUZZER_MODE_LOOP:
      // [循环模式] 响 -> 延时 -> 停 -> 延时 -> (继续循环)
      Buzzer_Beep(current_cmd.frequency, current_cmd.volume);
      osDelay(current_cmd.duration_on);

      Buzzer_Stop();
      osDelay(current_cmd.duration_off);
      break;

    case BUZZER_MODE_OFF:
    default:
      Buzzer_Stop();
      // OFF模式下，会在上面的 osMessageQueueGet 处阻塞挂起
      break;
    }
  }
  /* USER CODE END StartTask_Buzzer */
}

/* USER CODE BEGIN Header_StartTask_Servo */
/**
* @brief Function implementing the Task_Servo thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask_Servo */
void StartTask_Servo(void *argument)
{
  /* USER CODE BEGIN StartTask_Servo */
  uint8_t cmd;
  osStatus_t status;

  Servo_Init();

  /* Infinite loop */
  for(;;)
  {
    status = osMessageQueueGet(q_ServoCmdHandle, &cmd, NULL, osWaitForever);

    if (status == osOK)
    {
      if (cmd == SERVO_CMD_OPEN)
      {
        printf(">>> [SERVO] Action: OPEN Box\r\n");
        Servo_SetAngle(90);
      }
      else if (cmd == SERVO_CMD_CLOSE)
      {
        printf(">>> [SERVO] Action: Force CLOSE\r\n");
        Servo_SetAngle(0);
      }
    }
    osDelay(10);
  }
  /* USER CODE END StartTask_Servo */
}

/* USER CODE BEGIN Header_StartTask_RGB */
/**
* @brief Function implementing the Task_RGB thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask_RGB */
void StartTask_RGB(void *argument)
{
  /* USER CODE BEGIN StartTask_RGB */
  WS2812_Init();

  RGBCmd_t current_cmd = {RGB_MODE_OFF, 0, 0, 0, 0};
  RGBCmd_t new_cmd;
  osStatus_t status;

  int16_t breath_brightness = 0;
  int8_t breath_dir = 5;

  /* Infinite loop */
  for(;;)
  {
    status = osMessageQueueGet(q_RGBCmdHandle, &new_cmd, NULL, 10);

    if (status == osOK)
    {
      current_cmd = new_cmd;
      if (current_cmd.mode == RGB_MODE_BREATHING) {
        breath_brightness = 0;
        breath_dir = 5;
      }
    }

    switch (current_cmd.mode)
    {
      case RGB_MODE_STATIC:
        WS2812_SetAll(current_cmd.R, current_cmd.G, current_cmd.B);
        WS2812_Show();
        osDelay(100);
        break;

      case RGB_MODE_BLINK:
        WS2812_SetAll(current_cmd.R, current_cmd.G, current_cmd.B);
        WS2812_Show();
        osDelay(current_cmd.duration);
        WS2812_SetAll(0, 0, 0);
        WS2812_Show();
        osDelay(current_cmd.duration);
        break;

      case RGB_MODE_BREATHING:
        breath_brightness += breath_dir;
        if (breath_brightness >= current_cmd.G) { // 使用G作为最大亮度限制(Hack)
            breath_brightness = current_cmd.G;
            breath_dir = -1;
        }
        if (breath_brightness <= 0) {
            breath_brightness = 0;
            breath_dir = 1;
        }

        // 简单实现：只呼吸绿色通道，如果需要全彩呼吸需要更复杂算法
        // 这里假设传入的是 G 通道有值
        WS2812_SetAll(0, breath_brightness, 0);
        WS2812_Show();
        osDelay(20);
        break;

      case RGB_MODE_OFF:
      default:
        WS2812_SetAll(0, 0, 0);
        WS2812_Show();
        osDelay(500);
        break;
    }
  }
  /* USER CODE END StartTask_RGB */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

