
#include "tc74.h"

#include "spark_wiring_i2c.h"

/**
 * Cloud Function
 * Returns the temperature in celsius
 * @param  command unused
 * @return         temp in degrees celsius
 */
int getTempInCWrap(String command) 
{
	return getTempInC(TC74A0);
}

/**
 * Read the temperature from a TC74A# I2C temperature Sensor
 * @param  addr I2C address to read
 * @return      temperature in c
 */
int getTempInC(byte addr)
{
	int _temp = 255;
	// Get a reading from the temperature sensor
	Wire.beginTransmission(addr);
	Wire.write((uint8_t) 0);
	Wire.endTransmission();

	// request 1 byte from sensor
	Wire.requestFrom((int)addr,1);

	if (Wire.available())
	{
		_temp = Wire.read();
	}
	else
	{
		_temp = -22;
	}
	//handle sign
	if (_temp > 127) {
		_temp = 256 - _temp;
	}
	return _temp;
}