/*
* RC522.c
 * Adapted for STM32 HAL + FreeRTOS (SPI3)
 */

#include "RC522.h"
#include "stdio.h"
#include "spi.h"   // 必须包含，才能使用 hspi3

// --- 硬件接口宏定义 (根据你的最新CubeMX配置) ---

// 1. 片选引脚 (你命名为 RC522_SDA -> PD2)
// 注意：这里使用的是 RC522_SDA_GPIO_Port 和 RC522_SDA_Pin
#define RC522_CS_ENABLE   HAL_GPIO_WritePin(RC522_SDA_GPIO_Port, RC522_SDA_Pin, GPIO_PIN_RESET)
#define RC522_CS_DISABLE  HAL_GPIO_WritePin(RC522_SDA_GPIO_Port, RC522_SDA_Pin, GPIO_PIN_SET)

// 2. 复位引脚 (你命名为 RC522_RST -> PA15)
// 这里使用的是 RC522_RST_GPIO_Port 和 RC522_RST_Pin
#define RC522_RST_HIGH    HAL_GPIO_WritePin(RC522_RST_GPIO_Port, RC522_RST_Pin, GPIO_PIN_SET)
#define RC522_RST_LOW     HAL_GPIO_WritePin(RC522_RST_GPIO_Port, RC522_RST_Pin, GPIO_PIN_RESET)

// 声明外部的 SPI 句柄 (注意是 hspi3)
extern SPI_HandleTypeDef hspi3;

// ... 下面的代码保持不变 ...

/* 简单的微秒延时 (软件模拟) */
void delay_us(uint32_t nus)
{
    // 粗略延时，F411 主频100Mhz下，循环几次大概1us
    uint32_t i = nus * 10;
    while(i--);
}

/*
 * 核心函数：写寄存器
 * Address: 寄存器地址
 * value:   写入的值
 */
void WriteRawRC(unsigned char Address, unsigned char value)
{
    unsigned char ucAddr;
    uint8_t data[2];

    // RC522 SPI 写时序：地址左移1位，最低位为0
    ucAddr = ((Address << 1) & 0x7E);

    data[0] = ucAddr;
    data[1] = value;

    RC522_CS_ENABLE;
    // 使用 HAL 库直接发送2个字节 (地址 + 数据)
    HAL_SPI_Transmit(&hspi3, data, 2, 10);
    RC522_CS_DISABLE;
}

/*
 * 核心函数：读寄存器
 * Address: 寄存器地址
 * 返回值:   读到的值
 */
unsigned char ReadRawRC(unsigned char Address)
{
    unsigned char ucAddr;
    unsigned char ucResult = 0;

    // RC522 SPI 读时序：地址左移1位，最高位为1
    ucAddr = ((Address << 1) & 0x7E) | 0x80;

    RC522_CS_ENABLE;
    // 1. 发送地址
    HAL_SPI_Transmit(&hspi3, &ucAddr, 1, 10);
    // 2. 接收数据
    HAL_SPI_Receive(&hspi3, &ucResult, 1, 10);
    RC522_CS_DISABLE;

    return ucResult;
}

/* 初始化函数 */
void RC522_Init(void)
{
    // 1. 硬件复位
    RC522_CS_DISABLE; // 默认不选中
    RC522_RST_HIGH;
    HAL_Delay(10);
    RC522_RST_LOW;
    HAL_Delay(10);
    RC522_RST_HIGH;
    HAL_Delay(10);

    // 2. 软件复位
    PcdReset();

    // 3. 配置天线和模式
    PcdAntennaOff();
    HAL_Delay(10);
    PcdAntennaOn();

    // 设置模式，很重要
    WriteRawRC(ModeReg, 0x3D);
    WriteRawRC(TReloadRegL, 30);
    WriteRawRC(TReloadRegH, 0);
    WriteRawRC(TModeReg, 0x8D);
    WriteRawRC(TPrescalerReg, 0x3E);
    WriteRawRC(TxAutoReg, 0x40);

    printf("RFID Init Done.\r\n");
}

/* 复位 RC522 */
char PcdReset(void)
{
    RC522_RST_HIGH;
    delay_us(10);
    RC522_RST_LOW;
    HAL_Delay(10); // Use HAL Delay for ms
    RC522_RST_HIGH;
    delay_us(500);

    WriteRawRC(CommandReg, PCD_RESETPHASE);
    HAL_Delay(2);

    return MI_OK;
}

/* 开启天线 */
void PcdAntennaOn(void)
{
    unsigned char i;
    i = ReadRawRC(TxControlReg);
    if (!(i & 0x03))
    {
        SetBitMask(TxControlReg, 0x03);
    }
}

/* 关闭天线 */
void PcdAntennaOff(void)
{
    ClearBitMask(TxControlReg, 0x03);
}

/* 设置寄存器位 */
void SetBitMask(unsigned char reg, unsigned char mask)
{
    char tmp = 0x0;
    tmp = ReadRawRC(reg);
    WriteRawRC(reg, tmp | mask);
}

/* 清除寄存器位 */
void ClearBitMask(unsigned char reg, unsigned char mask)
{
    char tmp = 0x0;
    tmp = ReadRawRC(reg);
    WriteRawRC(reg, tmp & ~mask);
}

/* 寻卡 */
char PcdRequest(unsigned char req_code, unsigned char *pTagType)
{
    char status;
    unsigned int unLen;
    unsigned char ucComMF522Buf[MAXRLEN];

    ClearBitMask(Status2Reg, 0x08);
    WriteRawRC(BitFramingReg, 0x07);
    SetBitMask(TxControlReg, 0x03);

    ucComMF522Buf[0] = req_code;

    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 1, ucComMF522Buf, &unLen);

    if ((status == MI_OK) && (unLen == 0x10))
    {
        *pTagType = ucComMF522Buf[0];
        *(pTagType + 1) = ucComMF522Buf[1];
    }
    else
    {
        status = MI_ERR;
    }
    return status;
}

/* 防冲突 (获取卡号) */
char PcdAnticoll(unsigned char *pSnr)
{
    char status;
    unsigned char i, snr_check = 0;
    unsigned int unLen;
    unsigned char ucComMF522Buf[MAXRLEN];

    ClearBitMask(Status2Reg, 0x08);
    WriteRawRC(BitFramingReg, 0x00);
    ClearBitMask(CollReg, 0x80);

    ucComMF522Buf[0] = PICC_ANTICOLL1;
    ucComMF522Buf[1] = 0x20;

    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 2, ucComMF522Buf, &unLen);

    if (status == MI_OK)
    {
        for (i = 0; i < 4; i++)
        {
            *(pSnr + i) = ucComMF522Buf[i];
            snr_check ^= ucComMF522Buf[i];
        }
        if (snr_check != ucComMF522Buf[i])
        {
            status = MI_ERR;
        }
    }
    SetBitMask(CollReg, 0x80);
    return status;
}

/* 选卡 */
char PcdSelect(unsigned char *pSnr)
{
    char status;
    unsigned char i;
    unsigned int unLen;
    unsigned char ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = PICC_ANTICOLL1;
    ucComMF522Buf[1] = 0x70;
    ucComMF522Buf[6] = 0;

    for (i = 0; i < 4; i++)
    {
        ucComMF522Buf[i + 2] = *(pSnr + i);
        ucComMF522Buf[6] ^= *(pSnr + i);
    }
    CalulateCRC(ucComMF522Buf, 7, &ucComMF522Buf[7]);

    ClearBitMask(Status2Reg, 0x08);
    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 9, ucComMF522Buf, &unLen);

    if ((status == MI_OK) && (unLen == 0x18))
    {
        status = MI_OK;
    }
    else
    {
        status = MI_ERR;
    }
    return status;
}

/* 验证密码 */
char PcdAuthState(unsigned char auth_mode, unsigned char addr, unsigned char *pKey, unsigned char *pSnr)
{
    char status;
    unsigned int unLen;
    unsigned char i, ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = auth_mode;
    ucComMF522Buf[1] = addr;
    for (i = 0; i < 6; i++)
    {
        ucComMF522Buf[i + 2] = *(pKey + i);
    }
    for (i = 0; i < 6; i++)
    {
        ucComMF522Buf[i + 8] = *(pSnr + i);
    }

    status = PcdComMF522(PCD_AUTHENT, ucComMF522Buf, 12, ucComMF522Buf, &unLen);

    if ((status != MI_OK) || (!(ReadRawRC(Status2Reg) & 0x08)))
    {
        status = MI_ERR;
    }
    return status;
}

/* 读数据 */
char PcdRead(unsigned char addr, unsigned char *pData)
{
    char status;
    unsigned int unLen;
    unsigned char i, ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = PICC_READ;
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

    if ((status == MI_OK) && (unLen == 0x90))
    {
        for (i = 0; i < 16; i++)
        {
            *(pData + i) = ucComMF522Buf[i];
        }
    }
    else
    {
        status = MI_ERR;
    }
    return status;
}

/* 写数据 */
char PcdWrite(unsigned char addr, unsigned char *pData)
{
    char status;
    unsigned int unLen;
    unsigned char i, ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = PICC_WRITE;
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {
        status = MI_ERR;
    }

    if (status == MI_OK)
    {
        for (i = 0; i < 16; i++)
        {
            ucComMF522Buf[i] = *(pData + i);
        }
        CalulateCRC(ucComMF522Buf, 16, &ucComMF522Buf[16]);

        status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 18, ucComMF522Buf, &unLen);

        if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
        {
            status = MI_ERR;
        }
    }
    return status;
}

/* 休眠 */
char PcdHalt(void)
{
    unsigned int unLen;
    unsigned char ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = PICC_HALT;
    ucComMF522Buf[1] = 0;
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

    PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

    return MI_OK;
}

/* CRC计算 */
void CalulateCRC(unsigned char *pIndata, unsigned char len, unsigned char *pOutData)
{
    unsigned char i, n;

    ClearBitMask(DivIrqReg, 0x04);
    WriteRawRC(CommandReg, PCD_IDLE);
    SetBitMask(FIFOLevelReg, 0x80);

    for (i = 0; i < len; i++)
    {
        WriteRawRC(FIFODataReg, *(pIndata + i));
    }

    WriteRawRC(CommandReg, PCD_CALCCRC);

    i = 0xFF;
    do
    {
        n = ReadRawRC(DivIrqReg);
        i--;
    } while ((i != 0) && !(n & 0x04));

    pOutData[0] = ReadRawRC(CRCResultRegL);
    pOutData[1] = ReadRawRC(CRCResultRegM);
}

/* 与卡片通信底层协议 (核心) */
char PcdComMF522(unsigned char Command,
                 unsigned char *pInData,
                 unsigned char InLenByte,
                 unsigned char *pOutData,
                 unsigned int *pOutLenBit)
{
    char status = MI_ERR;
    unsigned char irqEn = 0x00;
    unsigned char waitFor = 0x00;
    unsigned char lastBits;
    unsigned char n;
    unsigned int i;

    switch (Command)
    {
    case PCD_AUTHENT:
        irqEn = 0x12;
        waitFor = 0x10;
        break;
    case PCD_TRANSCEIVE:
        irqEn = 0x77;
        waitFor = 0x30;
        break;
    default:
        break;
    }

    WriteRawRC(ComIEnReg, irqEn | 0x80);
    ClearBitMask(ComIrqReg, 0x80);
    WriteRawRC(CommandReg, PCD_IDLE);
    SetBitMask(FIFOLevelReg, 0x80);

    for (i = 0; i < InLenByte; i++)
    {
        WriteRawRC(FIFODataReg, pInData[i]);
    }

    WriteRawRC(CommandReg, Command);

    if (Command == PCD_TRANSCEIVE)
    {
        SetBitMask(BitFramingReg, 0x80);
    }

    // 等待接收，防止死循环
    i = 2000;
    do
    {
        n = ReadRawRC(ComIrqReg);
        i--;
    } while ((i != 0) && !(n & 0x01) && !(n & waitFor));

    ClearBitMask(BitFramingReg, 0x80);

    if (i != 0)
    {
        if (!(ReadRawRC(ErrorReg) & 0x1B))
        {
            status = MI_OK;
            if (n & irqEn & 0x01)
            {
                status = MI_NOTAGERR;
            }

            if (Command == PCD_TRANSCEIVE)
            {
                n = ReadRawRC(FIFOLevelReg);
                lastBits = ReadRawRC(ControlReg) & 0x07;
                if (lastBits)
                {
                    *pOutLenBit = (n - 1) * 8 + lastBits;
                }
                else
                {
                    *pOutLenBit = n * 8;
                }
                if (n == 0)
                {
                    n = 1;
                }
                if (n > MAXRLEN)
                {
                    n = MAXRLEN;
                }
                for (i = 0; i < n; i++)
                {
                    pOutData[i] = ReadRawRC(FIFODataReg);
                }
            }
        }
        else
        {
            status = MI_ERR;
        }
    }

    SetBitMask(ControlReg, 0x80);
    WriteRawRC(CommandReg, PCD_IDLE);

    return status;
}