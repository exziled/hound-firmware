/*!
 * @file socket_control.cpp
 * 
 * @brief HOUND Socket Control Abstraction Implementation
 * 
 * @author Benjamin Carlson
 * @author Blake Bourque
 * 
 * @date November 20, 2014
 * 
 * Function abstractions for socket control and data requests
 * 
 */

// Standard libraries
// HOUND Libraries
#include "socket_control.h"
// ST Libraries
#include "stm32f10x_gpio.h"


extern volatile socketMap_t socketMap[];

void initializeSocket(uint16_t socket)
{
	// Configure GPIO Pins for SPI Output
	GPIO_InitTypeDef pinInit;
    // TODO: RCC Init

	/* SPI Current CS Init */
    pinInit.GPIO_Pin = 1 << socketMap[socket].currentCSPin;
    pinInit.GPIO_Speed = GPIO_Speed_50MHz;
    pinInit.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(socketMap[socket].currentCSPort, &pinInit);
    GPIO_SetBits(socketMap[socket].currentCSPort, 1 << socketMap[socket].currentCSPin);	// Drive CS High to deactivate

	/* SPI Voltage CS Init */
    pinInit.GPIO_Pin = 1 << socketMap[socket].voltageCSPin;
    pinInit.GPIO_Speed = GPIO_Speed_50MHz;
    pinInit.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(socketMap[socket].voltageCSPort, &pinInit);
    GPIO_SetBits(socketMap[socket].voltageCSPort, 1 << socketMap[socket].voltageCSPin);	// Drive CS High to deactivate

	/* Triac Control Pin Init */
    pinInit.GPIO_Pin = 1 << socketMap[socket].controlPin;
    pinInit.GPIO_Speed = GPIO_Speed_50MHz;
    pinInit.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(socketMap[socket].controlPort, &pinInit);

    /* Create Conversion Storage Buffer */
    socketMap[socket].rmsResults = new AggregatedRMS(6);
}

void socketSetState(uint16_t socket, bool socketState)
{
	if (socketState)
	{
		GPIO_SetBits(socketMap[socket].controlPort, 1 << socketMap[socket].controlPin);
	} else {
		GPIO_ResetBits(socketMap[socket].controlPort, 1 << socketMap[socket].controlPin);
	}
}

void socketToggleState(uint16_t socket)
{

}


uint8_t socketGetState(uint16_t socket)
{
	// TODO: If the port is forced off due to overtemperature condition, that goes here
	return GPIO_ReadInputDataBit(socketMap[socket].controlPort, 1 << socketMap[socket].controlPin);
}

volatile socketMap_t * socketGetStruct(uint16_t socket)
{
    return &socketMap[socket];
}