#include "socket_control.h"

extern const socketMap_t socketMap[];

void initializeSocket(uint16_t socket)
{
	// Configure GPIO Pins for SPI Output
	GPIO_InitTypeDef pinInit;
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


int16_t socketGetCurrent(uint16_t socket)
{
	int16_t temp;

    GPIO_ResetBits(socketMap[socket].controlPort, 1 << socketMap[socket].controlPin);

    // Get CHA Data
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);

    SPI_I2S_SendData(SPI1, 0);

    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);

    // TODO: Change when we switch ADCs
    temp = SPI_I2S_ReceiveData(SPI1);
    temp = (temp & 0xFFF) | ((temp & 0x800) ? 0xF000 : 0);	// Sign extend
}