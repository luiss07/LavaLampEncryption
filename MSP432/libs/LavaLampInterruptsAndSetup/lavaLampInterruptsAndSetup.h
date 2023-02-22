#ifndef LAVALAMPINTERRUPTS_H
#define LAVALAMPINTERRUPTS_H

#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include "LcdDriver/HAL_MSP_EXP432P401R_Crystalfontz128x128_ST7735.h"

#include "../shared.h"

#define BP1_PORT GPIO_PORT_P5
#define BP1_PIN GPIO_PIN1
#define BP2_PORT GPIO_PORT_P3
#define BP2_PIN GPIO_PIN5

/*
 *  MCLK = HSMCLK = SMCLK = DCO of 24MHz
 *
 *               MSP432P401
 *             -----------------
 *            |                 |
 *       RST -|     P3.3/UCA0TXD|----|
 *            |                 |    |
 *           -|                 |    |
 *            |     P3.2/UCA0RXD|----|
 *            |                 |
 */

#define RX_BUF_SIZE 32

void _hwInit();

#endif