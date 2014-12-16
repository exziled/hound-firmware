/*!
 * @file hound_adc.cpp
 * 
 * @brief HOUND ADC Setup and Sampling Implemntation
 * 
 * @author Benjamin Carlson
 * @author Blake Bourque
 * 
 * @date August 27, 2014
 * 
 *
 * Functional implementation for low level SPI ADC interactions.  Also includes
 * implementation of TIM3 IRQ Handler, triggering individual samples
 * 
 */

// Standard Libraries
#include <cstddef>
#include <string.h>

// HOUND Libraries
#include "hound_def.h"
#include "hound_adc.h"
#include "hound_rms_fixed.h"
#include "hound_alignment.h"
#include "hound_debug.h"

// ST Libraries
#include "misc.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_rtc.h"

// Shared Interrupt/User Variables
static volatile sampleSetup_t * g_sConfig = NULL;
volatile int g_sActive = 0;

void initADCSPI(void)
{
    // Configure GPIO Pins for SPI Output
    GPIO_InitTypeDef pinInit;

    // Enable clock for SPI pins
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    // MISO, MOSI, SCK
    // PA 7 - MISO
    // PA 6 - MOSI
    // PA 5 - SCK
    pinInit.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_6 | GPIO_Pin_5;
    pinInit.GPIO_Speed = GPIO_Speed_50MHz;
    pinInit.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &pinInit);

    // Get SPI Clock Going
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

    SPI_InitTypeDef spiInit;
    spiInit.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spiInit.SPI_Mode = SPI_Mode_Master;
    spiInit.SPI_DataSize = SPI_DataSize_16b;
    spiInit.SPI_CPOL = SPI_CPOL_High;
    spiInit.SPI_CPHA = SPI_CPHA_2Edge;
    spiInit.SPI_NSS = SPI_NSS_Soft;
    spiInit.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;
    spiInit.SPI_FirstBit = SPI_FirstBit_MSB;
    spiInit.SPI_CRCPolynomial = 7;

    SPI_Init(SPI1, &spiInit);   // Set SPI Params
    SPI_Cmd(SPI1, ENABLE);      // Enable SPI

    // Sample Interval Timer
    initTIM3();
}

void initTIM3(void)
{
    //Enable Interrupt Generation
    NVIC_InitTypeDef tim3NVIC;

    tim3NVIC.NVIC_IRQChannel = TIM3_IRQn;
    tim3NVIC.NVIC_IRQChannelPreemptionPriority = 0;
    tim3NVIC.NVIC_IRQChannelSubPriority = 0;
    tim3NVIC.NVIC_IRQChannelCmd = ENABLE;

    NVIC_Init(&tim3NVIC);

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    // Create init structure and initialize to default values
    TIM_TimeBaseInitTypeDef timeInit;
    TIM_TimeBaseStructInit(&timeInit);

    timeInit.TIM_Prescaler = 3;
    timeInit.TIM_CounterMode = TIM_CounterMode_Up;
    timeInit.TIM_Period = 1498;
    timeInit.TIM_ClockDivision = TIM_CKD_DIV1;
    timeInit.TIM_RepetitionCounter = 0;

    TIM_TimeBaseInit(TIM3, &timeInit);

    TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

    TIM_Cmd(TIM3, ENABLE);
}


int getSampleBlock(volatile sampleSetup_t * sampleSetup)
{
    // First run, don't bother to de-init anything
    if (g_sConfig != NULL)
    {
        // Don't accecpt a new job if current isn't complete
        if (g_sConfig->bufferSize != g_sConfig->sampleCount)
        {
            return -1;
        } else {
            g_sActive = 0;
            // Do De-Init (like the CS pin)
        }
    }

    // Pass over the sample struct
    g_sConfig = sampleSetup;

    // Reset buffer index
    sampleSetup->sampleCount = 0;

    // Fire away, we'll get an interrupt when it's done
    // 400 samples @ 24ksps = 1/60 seconds to fill
    g_sActive = 1;

    // Success, job accecpted
    return 0;
}

extern "C" void TIM3_IRQHandler()
{
    int16_t temp = 0;

    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);

        // No sample configuration, something's very wrong
        if (g_sConfig == NULL)
        {
            return;
        }

        // While there's still data to process
        if (g_sConfig->sampleCount < g_sConfig->bufferSize)
        {
            #ifdef NEW_SAMPLE_BOARD
                if (g_sConfig->sampleCount == 0)
                {
                    // Sample Current
                    GPIO_ResetBits(g_sConfig->currentCSPort, 1 << g_sConfig->currentCSPin);

                    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
                    SPI_I2S_SendData(SPI1, g_sConfig->currentSPIAlt);   // Channel of next current sample
                    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
                    temp = SPI_I2S_ReceiveData(SPI1);
                    GPIO_SetBits(g_sConfig->currentCSPort, 1 << g_sConfig->currentCSPin);
                    // Sample Voltage
                    GPIO_ResetBits(g_sConfig->voltageCSPort, 1 << g_sConfig->voltageCSPin);
                    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
                    SPI_I2S_SendData(SPI1, 0);   // No channels for voltage
                    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
                    temp = SPI_I2S_ReceiveData(SPI1);
                    GPIO_SetBits(g_sConfig->voltageCSPort, 1 << g_sConfig->voltageCSPin);
                }

                // Sample Current
                GPIO_ResetBits(g_sConfig->currentCSPort, 1 << g_sConfig->currentCSPin);

                /* Code assumes that previous read correctly informed ADC
                 * the channel of this read.  Assume we're going to do the same
                 */
                while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
                SPI_I2S_SendData(SPI1, g_sConfig->currentSPIAlt);   // Channel of next current sample
                while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
                

                // Hopefully, recieved data is correct channel
                temp = SPI_I2S_ReceiveData(SPI1);
                temp = temp & 0xFFF;

                g_sConfig->currentBuffer[g_sConfig->sampleCount] = fixed(temp);
                GPIO_SetBits(g_sConfig->currentCSPort, 1 << g_sConfig->currentCSPin);

                // Sample Voltage
                GPIO_ResetBits(g_sConfig->voltageCSPort, 1 << g_sConfig->voltageCSPin);
                while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
                SPI_I2S_SendData(SPI1, 0);   // No channels for voltage
                while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
                
                temp = SPI_I2S_ReceiveData(SPI1);
                temp = temp >> 3;
                // temp = temp & 0x3FF;

                g_sConfig->voltageBuffer[g_sConfig->sampleCount] = fixed(temp);
                GPIO_SetBits(g_sConfig->voltageCSPort, 1 << g_sConfig->voltageCSPin);

            #else
                // Drive CS Low
                GPIO_ResetBits(g_sConfig->currentCSPort, 1 << g_sConfig->currentCSPin);

                // Get CHA (Current) Data
                while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
                SPI_I2S_SendData(SPI1, 0);
                while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);

                temp = SPI_I2S_ReceiveData(SPI1);
                temp = (temp & 0xFFF) | ((temp & 0x800) ? 0xF000 : 0);  // Sign extend

                g_sConfig->currentBuffer[g_sConfig->sampleCount] = fixed(temp);


                // Get CHB (Voltage) Data
                while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
                SPI_I2S_SendData(SPI1, 0);
                while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);

                temp = SPI_I2S_ReceiveData(SPI1);
                temp = (temp & 0xFFF) | ((temp & 0x800) ? 0xF000 : 0);  // Sign Extend

                g_sConfig->voltageBuffer[g_sConfig->sampleCount] = fixed(temp);

                // Transfer Done, CS High
                GPIO_SetBits(g_sConfig->currentCSPort, 1 << g_sConfig->currentCSPin);
            #endif

            g_sConfig->sampleCount++;

            // Trigger our lower priority calculation loop
            if (g_sConfig->sampleCount == g_sConfig->bufferSize)
            {
                rmsValues_t * rmsValues;

                #ifdef NEW_SAMPLE_BOARD
                    // Make "3.3V"
                    fixed_t current_mul = fixed(3);
                    current_mul = fixed_mul(current_mul, fixed(11));
                    current_mul = fixed_div(current_mul, fixed(10));

                    fixed_t current_offset = fixed(alignHOUND_currentReference(4095) + 35);
                    current_offset = fixed_mul(current_offset, 66);
                    current_offset = fixed_div(current_offset, 100);
                #endif

                //fixed_t current_offset = alignHOUND_HallEffectSetOffset(g_sConfig->socket_id, g_sConfig->currentBuffer, g_sConfig->bufferSize);

                //HoundDebug::logError(current_offset >> 16, "Current Offset");

                for (int i = 0; i < g_sConfig->bufferSize; i++)
                {
                    #ifdef NEW_SAMPLE_BOARD
                        // Current
                        // ADC Representation of 0.95V (our "0" offset);
                        g_sConfig->currentBuffer[i] = fixed_sub(g_sConfig->currentBuffer[i], current_offset);
                        // // 12 Bit ADC
                        g_sConfig->currentBuffer[i] = fixed_div(g_sConfig->currentBuffer[i], fixed(4095));
                        // 3.3V Input
                        g_sConfig->currentBuffer[i] = fixed_mul(g_sConfig->currentBuffer[i], current_mul);

                        g_sConfig->currentBuffer[i] = fixed_mul(g_sConfig->currentBuffer[i], fixed(16));

                        g_sConfig->currentBuffer[i] = fixed_div(g_sConfig->currentBuffer[i], fixed(10));

                        // Voltage
                        // "0" Offset is 2.5V //512
                        g_sConfig->voltageBuffer[i] = fixed_sub(g_sConfig->voltageBuffer[i], fixed(512));
                        // 10 Bit ADC
                        g_sConfig->voltageBuffer[i] = fixed_div(g_sConfig->voltageBuffer[i], fixed(1024));                    
                        // Scaling and voltage refernce
                        g_sConfig->voltageBuffer[i] = fixed_mul(g_sConfig->voltageBuffer[i], fixed(71 * 5));
                        // DSP Scaling
                        g_sConfig->voltageBuffer[i] = fixed_div(g_sConfig->voltageBuffer[i], fixed(100));

                    #else            
                        g_sConfig->currentBuffer[i] = fixed_div(g_sConfig->currentBuffer[i], fixed(4095));
                        g_sConfig->currentBuffer[i] = fixed_mul(g_sConfig->currentBuffer[i], fixed(50));
                        g_sConfig->currentBuffer[i] = fixed_div(g_sConfig->currentBuffer[i], fixed(10));

                        g_sConfig->voltageBuffer[i] = fixed_div(g_sConfig->voltageBuffer[i], fixed(4095));
                        g_sConfig->voltageBuffer[i] = fixed_mul(g_sConfig->voltageBuffer[i], fixed(86 * 5));
                        g_sConfig->voltageBuffer[i] = fixed_div(g_sConfig->voltageBuffer[i], fixed(100));
                    #endif
                    
                }

                // Get latest samples
                rmsValues = g_sConfig->rmsResults->getAt(0);

                uint32_t newTimestamp = RTC_GetCounter();
                fixed_t tempCurrent, tempVoltage;

                // Move to next aggregation index depending on time
                if ( (newTimestamp - g_sConfig->rmsResults->getAt(1)->timestamp) > (60 / (g_sConfig->rmsResults->getSize() - 1)))
                {
                    rmsValues = g_sConfig->rmsResults->getBack();
                }
                rmsValues->timestamp = newTimestamp;

                // Calculate RMS current and voltage values
                calc_rms(g_sConfig->voltageBuffer, BLOCKSIZE, (fixed_t *)&(tempVoltage));
                calc_rms(g_sConfig->currentBuffer, BLOCKSIZE, (fixed_t *)&(tempCurrent));

                rmsValues->voltage = fixed_mul(fixed(100), tempVoltage);
                rmsValues->current = fixed_mul(fixed(10), tempCurrent);

                rmsValues->apparent = fixed_mul(rmsValues->voltage, rmsValues->current);

                fixed_t fixed_scale = fixed(1);
                fixed_scale = fixed_div(fixed_scale, 12000); 
                fixed_t sum = 0;

                for (int i = 0; i < g_sConfig->bufferSize; i++)
                {
                    // Real power is individual current and voltage samples multiplied
                    sum += (fixed_mul(fixed_mul(g_sConfig->voltageBuffer[i], g_sConfig->currentBuffer[i]), fixed_scale));
                }

                rmsValues->real = fixed_mul(sum, 60);
                rmsValues->real = fixed_mul(fixed(100*10), rmsValues->real);
                if ((int32_t)rmsValues->real < 0)
                {
                    rmsValues->real = 0;
                }

                rmsValues->pf = fixed_div(rmsValues->real, rmsValues->apparent);
                if ((int32_t)rmsValues->pf < 0)
                {
                    rmsValues->pf = 0;
                }
            }
        }

    }
}