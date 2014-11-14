#ifndef TC74A_H_
#define TC74A_H_

#include "application.h"

#define TC74A0 0x48
#define TC74A1 0x49
#define TC74A2 0x4a
#define TC74A3 0x4b
#define TC74A4 0x4c
#define TC74A5 0x4d
#define TC74A6 0x4e
#define TC74A7 0x4f

int getTempInCWrap(String command);
int getTempInC(byte addr);

#endif /* TC74A_H_ */