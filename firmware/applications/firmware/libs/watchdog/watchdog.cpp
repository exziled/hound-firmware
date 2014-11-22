#include "watchdog.h"

#include "stm32f10x_rcc.h"
#include "stm32f10x_iwdg.h"

void enableWatchdog(void)
{
	/* Enable WWDG clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_WWDG, ENABLE);

	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

	/* IWDG clock counter = LCI (40kHz / 128) = 312Hz (3.2ms) */
	IWDG_SetPrescaler(IWDG_Prescaler_128);

	/* 625 ticks (2 seconds) before watchdog reset */
	IWDG_SetReload(625);

	IWDG_Enable();
}

void disableWatchdog(void)
{
	/* Enable WWDG clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_WWDG, DISABLE);
}

void updateWatchdog(void)
{
	IWDG_ReloadCounter();
	return;
}

