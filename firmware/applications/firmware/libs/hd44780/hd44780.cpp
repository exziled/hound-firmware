/*!
 * @file
 *
 * @brief STM32F* HD44780 LCD Driver Implementation
 *
 * @author Benjamin Carlson
 *
 * @date Aug 20, 2014
 *
 * Wrapper class for initialization and communcation with HD44780 series (and
 * compatible) 2x16 character LCD screens.
 */

#include "hd44780.h"

#include <stdio.h>
#include <stdarg.h>

#include "stm32f10x_gpio.h"


HD44780::HD44780()
{
    //pinConfig = pinConfig;
    // TODO: Just use member variable
    //initLCD(pinConfig);
}

HD44780::~HD44780(void)
{
    // Eventually do something to de-init pins and cleanup after ourselves
}

HD44780 * HD44780::getInstance(LCDPinConfig_t* pinConfig)
{
    static HD44780 lcd_instance;
    
    if (pinConfig != NULL)
    {
        lcd_instance.initLCD(pinConfig);
    }

    return &lcd_instance;
}

void HD44780::initLCD(LCDPinConfig_t* pinConfig)
{
    m_pinConfig.portRS = pinConfig->portRS;
    m_pinConfig.pinRS = pinConfig->pinRS;
    m_pinConfig.portEnable = pinConfig->portEnable;
    m_pinConfig.pinEnable = pinConfig->pinEnable;
    m_pinConfig.portRW = pinConfig->portRW;
    m_pinConfig.pinRW = pinConfig->pinRW;
    m_pinConfig.portData0 = pinConfig->portData0;
    m_pinConfig.pinData0 = pinConfig->pinData0;
    m_pinConfig.portData1 = pinConfig->portData1;
    m_pinConfig.pinData1 = pinConfig->pinData1;
    m_pinConfig.portData2 = pinConfig->portData2;
    m_pinConfig.pinData2 = pinConfig->pinData2;
    m_pinConfig.portData3 = pinConfig->portData3;
    m_pinConfig.pinData3 = pinConfig->pinData3;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
   
    // Configure Enable Port
    GPIO_InitTypeDef enableInit;
    enableInit.GPIO_Pin = pinConfig->pinEnable;
    enableInit.GPIO_Speed = GPIO_Speed_10MHz;
    enableInit.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(pinConfig->portEnable, &enableInit);

    // Configure RS
    GPIO_InitTypeDef RSInit;
    RSInit.GPIO_Pin = pinConfig->pinRS;
    RSInit.GPIO_Speed = GPIO_Speed_10MHz;
    RSInit.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(pinConfig->portRS, &RSInit);

    // Configure RW
    GPIO_InitTypeDef RWInit;
    RWInit.GPIO_Pin = pinConfig->pinRW;
    RWInit.GPIO_Speed = GPIO_Speed_10MHz;
    RWInit.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(pinConfig->portRW, &RWInit);

    // Configure 4 Data Lines
    GPIO_InitTypeDef DataInit;
    DataInit.GPIO_Pin = pinConfig->pinData0;
    DataInit.GPIO_Speed = GPIO_Speed_10MHz;
    DataInit.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(pinConfig->portData0, &DataInit);
    DataInit.GPIO_Pin = pinConfig->pinData1;
    DataInit.GPIO_Speed = GPIO_Speed_10MHz;
    DataInit.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(pinConfig->portData1, &DataInit);
    DataInit.GPIO_Pin = pinConfig->pinData2;
    DataInit.GPIO_Speed = GPIO_Speed_10MHz;
    DataInit.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(pinConfig->portData2, &DataInit);
    DataInit.GPIO_Pin = pinConfig->pinData3;
    DataInit.GPIO_Speed = GPIO_Speed_10MHz;
    DataInit.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(pinConfig->portData3, &DataInit);

    // Get All Pins to clean state
    GPIO_ResetBits(pinConfig->portEnable, pinConfig->pinEnable);
    GPIO_ResetBits(pinConfig->portRS, pinConfig->pinRS);
    GPIO_ResetBits(pinConfig->portRW, pinConfig->pinRW);
    GPIO_ResetBits(pinConfig->portData0, pinConfig->pinData0);
    GPIO_ResetBits(pinConfig->portData1, pinConfig->pinData1);
    GPIO_ResetBits(pinConfig->portData2, pinConfig->pinData2);
    GPIO_ResetBits(pinConfig->portData3, pinConfig->pinData3);

    delay_us(50000);
    // LCD Init Sequence

    GPIO_WriteBit(pinConfig->portData0, pinConfig->pinData0, Bit_RESET);
    GPIO_WriteBit(pinConfig->portData1, pinConfig->pinData1, Bit_SET);
    GPIO_WriteBit(pinConfig->portData2, pinConfig->pinData2, Bit_RESET);
    GPIO_WriteBit(pinConfig->portData3, pinConfig->pinData3, Bit_RESET);
    toggleEnable();
    GPIO_WriteBit(pinConfig->portData0, pinConfig->pinData0, Bit_RESET);
    GPIO_WriteBit(pinConfig->portData1, pinConfig->pinData1, Bit_SET);
    GPIO_WriteBit(pinConfig->portData2, pinConfig->pinData2, Bit_RESET);
    GPIO_WriteBit(pinConfig->portData3, pinConfig->pinData3, Bit_RESET);
    toggleEnable();
    GPIO_WriteBit(pinConfig->portData0, pinConfig->pinData0, Bit_RESET);
    GPIO_WriteBit(pinConfig->portData1, pinConfig->pinData1, Bit_RESET);
    GPIO_WriteBit(pinConfig->portData2, pinConfig->pinData2, Bit_RESET);
    GPIO_WriteBit(pinConfig->portData3, pinConfig->pinData3, Bit_SET);
    toggleEnable();

    delay_us(1000);
    writeData(0x0c);
    delay_us(1000);
    writeData(0x01);
    delay_us(1000);
    writeData(0x06);
    delay_us(1000);
    writeData(0x02);
    delay_us(1000);
    writeData(0x0F);
    delay_us(5000);
}

void HD44780::writeData(char c)
{
    // Output upper bits first, then toggle enable
    GPIO_WriteBit(m_pinConfig.portData0, m_pinConfig.pinData0, ((c & 0x10) ? Bit_SET : Bit_RESET));
    GPIO_WriteBit(m_pinConfig.portData1, m_pinConfig.pinData1, ((c & 0x20) ? Bit_SET : Bit_RESET));
    GPIO_WriteBit(m_pinConfig.portData2, m_pinConfig.pinData2, ((c & 0x40) ? Bit_SET : Bit_RESET));
    GPIO_WriteBit(m_pinConfig.portData3, m_pinConfig.pinData3, ((c & 0x80) ? Bit_SET : Bit_RESET));

    toggleEnable();

    // Output lower bits, again toggle enable
    GPIO_WriteBit(m_pinConfig.portData0, m_pinConfig.pinData0, ((c & 0x1) ? Bit_SET : Bit_RESET));
    GPIO_WriteBit(m_pinConfig.portData1, m_pinConfig.pinData1, ((c & 0x2) ? Bit_SET : Bit_RESET));
    GPIO_WriteBit(m_pinConfig.portData2, m_pinConfig.pinData2, ((c & 0x4) ? Bit_SET : Bit_RESET));
    GPIO_WriteBit(m_pinConfig.portData3, m_pinConfig.pinData3, ((c & 0x8) ? Bit_SET : Bit_RESET));

    toggleEnable();
}

void HD44780::toggleEnable(void)
{
    GPIO_SetBits(m_pinConfig.portEnable, m_pinConfig.pinEnable);
    delay_us(150);
    GPIO_ResetBits(m_pinConfig.portEnable, m_pinConfig.pinEnable);
    delay_us(1000);
}

void HD44780::writeCharacter(char c)
{
    GPIO_SetBits(m_pinConfig.portRS, m_pinConfig.pinRS);
    delay_us(500);
    writeData(c);
}

void HD44780::printf(char * fmt, ...)
{
    char buff[40];

    va_list args;
    va_start(args, fmt);

    vsprintf(buff, fmt, args);

    va_end(args);

    writeString(buff);
}

void HD44780::writeString(char* string)
{
    while (*string)
    {
        writeCharacter(*string++);
    }
}

void HD44780::setPosition(uint8_t line, uint8_t position)
{
    uint8_t pos_relative;   // Handle position increase for 2nd line

    // RS = 0: Command Mode
    GPIO_ResetBits(m_pinConfig.portRS, m_pinConfig.pinRS);
    delay_us(500);

    switch (line)
    {
        case 0:
            pos_relative = 0;
            break;
        default:
            pos_relative = 0x40;
            break;
    }

    writeData(0x80 | (pos_relative | position));
}

void HD44780::clear(void)
{
    // RS = 0: Command Mode
    GPIO_ResetBits(m_pinConfig.portRS, m_pinConfig.pinRS);
    delay_us(500);

    writeData(0x01);

    delay_us(3000);
}

void HD44780::home(void)
{
    // RS = 0: Command Mode
    GPIO_ResetBits(m_pinConfig.portRS, m_pinConfig.pinRS);
    delay_us(500);

    writeData(0x02);

    delay_us(1500);
}

void HD44780::delay_us(uint32_t us)
{
    us *= 24;

    /* fudge for function call overhead  */
    //us--;
    asm volatile("   mov r0, %[us]          \n\t"
                 "1: subs r0, #1            \n\t"
                 "   bhi 1b                 \n\t"
                 :
                 : [us] "r" (us)
                 : "r0");
}
