#include "cloud_dfu.h"

/**
 * Cloud Function
 * This function puts the core in DFU mode for USB flashing
 * @param  command unused
 * @return         0 on success
 */
int doDFU(String command) {

	FLASH_OTA_Update_SysFlag = 0x0000;
	Save_SystemFlags();
	BKP_WriteBackupRegister(BKP_DR10, 0x0000);
	USB_Cable_Config(DISABLE);
	NVIC_SystemReset();

	return 0;
}