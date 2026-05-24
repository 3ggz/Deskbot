#pragma once

#include <stdio.h>
#include "I2C_Driver.h"

#define TCA9554_EXIO1 0x01
#define TCA9554_EXIO2 0x02
#define TCA9554_EXIO3 0x03
#define TCA9554_EXIO4 0x04
#define TCA9554_EXIO5 0x05
#define TCA9554_EXIO6 0x06
#define TCA9554_EXIO7 0x07

/****** TCA9554PWR information ******/

#define TCA9554_ADDRESS         0x20                      // TCA9554PWR I2C address
// TCA9554PWR register addresses
#define TCA9554_INPUT_REG       0x00                      // Input register, input level
#define TCA9554_OUTPUT_REG      0x01                      // Output register, high and low level output
#define TCA9554_Polarity_REG    0x02                      // Polarity Inversion register
#define TCA9554_CONFIG_REG      0x03                      // Configuration register, mode configuration


#define Low   0
#define High  1
#define EXIO_PIN1   1
#define EXIO_PIN2   2
#define EXIO_PIN3   3
#define EXIO_PIN4   4
#define EXIO_PIN5   5
#define EXIO_PIN6   6
#define EXIO_PIN7   7
#define EXIO_PIN8   8

/*** Operation register REG ***/
uint8_t I2C_Read_EXIO(uint8_t REG);
uint8_t I2C_Write_EXIO(uint8_t REG, uint8_t Data);
/*** Set EXIO mode ***/
void Mode_EXIO(uint8_t Pin, uint8_t State);
void Mode_EXIOS(uint8_t PinState);
/*** Read EXIO status ***/
uint8_t Read_EXIO(uint8_t Pin);
uint8_t Read_EXIOS(uint8_t REG);
/*** Set the EXIO output status ***/
void Set_EXIO(uint8_t Pin, uint8_t State);
void Set_EXIOS(uint8_t PinState);
/*** Flip EXIO state ***/
void Set_Toggle(uint8_t Pin);
/*** TCA9554PWR Init ***/
void TCA9554PWR_Init(uint8_t PinState = 0x00);
