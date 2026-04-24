#include <ch32fun.h>
#include "lib_i2c.h"

#include "sc7a20.h"


// Initialization values for control registers, in reg-value pairs
// Basically setup the unit to run, and enable 4D orientation detection
// TODO: Make better comments and define values
static const uint8_t i2c_registers[][2] = {
    {    SC7A20_CTRL_REG1, 0b01100111}, // 200Hz, XYZ enabled
    {    SC7A20_CTRL_REG2, 0b00000000}, // Setup filter to 0x00 ??
    {    SC7A20_CTRL_REG3, 0b00000000}, // INT1 off
    {    SC7A20_CTRL_REG4, 0b00001000}, // Block mode off,little-endian,2G,High-pres,self test off
    {    SC7A20_CTRL_REG5, 0b00000100}, // fifo off, D4D on INT1
    {    SC7A20_CTRL_REG6,       0x00}, // INT2 off
    {     SC7A20_INT2_CFG, 0b01111110}, // setup for movement detection
    {     SC7A20_INT2_THS,       0x28}, //
    {SC7A20_INT2_DURATION,         64}, //
    {     SC7A20_INT1_CFG, 0b01111110}, //
    {     SC7A20_INT1_THS,       0x28}, //
    {SC7A20_INT1_DURATION,         64}
};


void sc7a20_init()
{
	// Setup acceleration readings
	// 2G range
	// bandwidth = 250Hz
	// High pass filter on (Slow compensation)
	// Turn off IRQ output pins
	// Orientation recognition in symmetrical mode
	// Hysteresis is set to ~ 16 counts
	// Theta blocking is set to 0b10
	for (uint8_t i = 0; i < sizeof(i2c_registers) / sizeof(i2c_registers[0]); i++) {
		uint8_t bytes[2] = {i2c_registers[i][0], i2c_registers[i][1]};
		i2c_sendBytes(I2C_TARGET, SC7A20_ADDRESS, bytes, 2);
	}
}

void sc7a20_get_readings(int16_t *x, int16_t *y, int16_t *z)
{
	// We can tell the accelerometer to output in LE mode which makes this simple
	int16_t sensorData[3] = {0};

	i2c_sendByte(I2C_TARGET, SC7A20_ADDRESS, SC7A20_OUT_X_L | 0x80);
	i2c_readBytes(I2C_TARGET, SC7A20_ADDRESS, (u8*)sensorData, 6);

	// Lower byte is sent first
	*x = (int16_t)sensorData[0];
	*y = (int16_t)sensorData[1];
	*z = (int16_t)sensorData[2];
}
