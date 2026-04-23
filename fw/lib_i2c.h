#ifndef _LIB_I2C_H
#define _LIB_I2C_H

// MIT License
// Copyright (c) 2025 UniTheCat
// Tested with Ch32X03x and CH32V30x

#include <ch32fun.h>


#define I2C_DEFAULT_TIMEOUT 100000

//! ####################################
//! I2C INIT FUNCTIONS
//! ####################################

void i2c_init(I2C_TypeDef* I2Cx, u32 PCLK, u32 i2cSpeed_Hz);
u8 i2c_start(I2C_TypeDef* I2Cx, u8 i2cAddress, u8 isRead);
void i2c_stop(I2C_TypeDef* I2Cx);
void i2c_scan(I2C_TypeDef* I2Cx, void (*onPingFound)(u8 address));

//! ####################################
//! I2C SEND FUNCTION
//! ####################################

u8 i2c_sendBytes_noStop(I2C_TypeDef* I2Cx, u8 i2cAddress, const u8* buffer, u8 len);
u8 i2c_sendBytes(I2C_TypeDef* I2Cx, u8 i2cAddress, const u8* buffer, u8 len);
u8 i2c_sendByte(I2C_TypeDef* I2Cx, u8 i2cAddress, u8 data);

//! ####################################
//! I2C RECEIVE FUNCTIONS
//! ####################################

u8 i2c_readBytes(I2C_TypeDef* I2Cx, u8 i2cAddress, u8* buffer, u8 len);
u8 i2c_readByte(I2C_TypeDef* I2Cx, u8 i2cAddress, u8 reg, u8* buffer);

// Write to register and then do read data, no stop inbetween
u8 i2c_readRegTx_buffer(I2C_TypeDef* I2Cx, u8 i2cAddress, u8 *tx_buf, u8 tx_len, u8 *rx_buf, u8 rx_len);
inline u8 i2c_readReg_buffer(I2C_TypeDef* I2Cx, u8 i2cAddress, u8 reg, u8 *rx_buf, u8 rx_len)
{
	return i2c_readRegTx_buffer(I2Cx, i2cAddress, &reg, 1, rx_buf, rx_len);
}

//! ####################################
//! I2C SLAVE FUNCTIONS
//! ####################################

void i2c_slave_init(I2C_TypeDef* I2Cx, u16 self_addr, u32 PCLK, u32 i2cSpeed_Hz);


#endif // _LIB_I2C_H
