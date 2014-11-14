/*!
 * @file
 *
 * @brief STM32F* HD44780 LCD Driver - 4 Bit Data Transfer
 *
 * @author Benjamin Carlson
 *
 * @date Aug 20, 2014
 *
 * Wrapper class for initialization and communcation with HD44780 series (and
 * compatible) 2x16 character LCD screens.
 */

#include "stm32f10x.h"

#define STM32_DELAY_US_MULT 24 // 72/3

 typedef struct
 {
   GPIO_TypeDef* portRS;
   uint16_t pinRS;
   GPIO_TypeDef* portEnable;
   uint16_t pinEnable;
   GPIO_TypeDef* portRW;
   uint16_t pinRW;
   GPIO_TypeDef* portData0;
   uint16_t pinData0;
   GPIO_TypeDef* portData1;
   uint16_t pinData1;
   GPIO_TypeDef* portData2;
   uint16_t pinData2;
   GPIO_TypeDef* portData3;
   uint16_t pinData3;
 } LCDPinConfig_t;

 class HD44780
 {
 public:
   static HD44780 * getInstance(LCDPinConfig_t* pinConfig = 0);

   ~HD44780(void);

   void printf(char * fmt, ...);
   void writeString(char *string);
   void writeCharacter(char c);

   void setPosition(uint8_t line, uint8_t position);

   void clear(void);
   void home(void);

   void delay_us(uint32_t us);

   void initLCD(LCDPinConfig_t* pinConfig);

 private:
   HD44780();

   void writeData(char c);
   void toggleEnable();


   HD44780 * m_Instance = 0;
   LCDPinConfig_t m_pinConfig;
 };
