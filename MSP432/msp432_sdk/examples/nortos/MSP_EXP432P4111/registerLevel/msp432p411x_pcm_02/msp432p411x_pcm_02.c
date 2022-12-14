/* --COPYRIGHT--,BSD_EX
 * Copyright (c) 2013, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************
 *
 *                       MSP432 CODE EXAMPLE DISCLAIMER
 *
 * MSP432 code examples are self-contained low-level programs that typically
 * demonstrate a single peripheral function or device feature in a highly
 * concise manner. For this the code may rely on the device's power-on default
 * register values and settings such as the clock configuration and care must
 * be taken when combining code from several examples to avoid potential side
 * effects. Also see http://www.ti.com/tool/mspdriverlib for an API functional
 * library & https://dev.ti.com/pinmux/ for a GUI approach to peripheral configuration.
 *
 * --/COPYRIGHT--*/
//******************************************************************************
//   MSP432P4111 Demo - Enter LPM3 (ARM Deep Sleep Mode) with ACLK = REFO
//
//  Description: MSP432 device is configured to enter LPM3 mode with GPIOs properly
//  terminated. P1.1 is configured as an input. Pressing the button connect to P1.1
//  results in device waking up and servicing the Port1 ISR. LPM3 current can be
//  measured when P1.0 is output low (e.g. LED off).
//  Added while(1) to allow repeated LPM3 entry.
//  Example now puts most SRARM in retention
//
//  ACLK = 32kHz, MCLK = SMCLK = default DCO
//
//
//               MSP432P411x
//            -----------------
//        /|\|                 |
//         | |                 |
//         --|RST              |
//     /|\   |                 |
//      --o--|P1.1         P1.0|-->LED
//     \|/
//
//   Bob Landers
//   Texas Instruments Inc.
//   November 2017 (updated) | November 2013 (created)
//   Built with CCSv7.3, IAR 8.20.1, Keil 5.24.1, GCC 6.3.1
//******************************************************************************
#include "ti/devices/msp432p4xx/inc/msp.h"

int main(void)
{
    // Halting the Watchdog
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;

    // Terminate all pins on the device to minimize power consumption by default
    P1->DIR |= 0xFF; P1->OUT = 0;
    P2->DIR |= 0xFF; P2->OUT = 0;
    P3->DIR |= 0xFF; P3->OUT = 0;
    P4->DIR |= 0xFF; P4->OUT = 0;
    P5->DIR |= 0xFF; P5->OUT = 0;
    P6->DIR |= 0xFF; P6->OUT = 0;
    P7->DIR |= 0xFF; P7->OUT = 0;
    P8->DIR |= 0xFF; P8->OUT = 0;
    P9->DIR |= 0xFF; P9->OUT = 0;
    P10->DIR |= 0xFF; P10->OUT = 0;
    PJ->DIR |= 0xFF; PJ->OUT = 0;

    // Configuring P1.1 (switch) as input with pull-up resistor
    // Notice intentional '=' assignment since all P1 pins are being deliberately
    // configured.
    P1->DIR = ~(BIT1);
    P1->OUT = BIT1;
    P1->REN = BIT1;                         // Enable pull-up resistor (P1.1 output high)
    P1->SEL0 = 0;
    P1->SEL1 = 0;
    P1->IFG = 0;                            // Clear all P1 interrupt flags
    P1->IE = BIT1;                          // Enable interrupt for P1.1
    P1->IES = BIT1;                         // Interrupt on high-to-low transition

    // Enable Port 1 interrupt on the NVIC
    NVIC->ISER[1] = 1 << ((PORT1_IRQn) & 31);
											
    // Configure P5.3 as ADC-in. Avoid driving this pin on the MSP432P4111
    //   LaunchPad since it's connected to the LMT70 temp sensor.  
    //   Driving P5.3 as a digital-low can result in 1mA+ of current on Vcc.
    //   Note: This isn't necessary if you've removed R8 on the LaunchPad
    P5->SEL1 |= BIT3;
    P5->SEL0 |= BIT3;

    // Select REFO for ACLK
    CS->KEY = CS_KEY_VAL;
    CS->CTL1 &= ~(CS_CTL1_SELA_MASK);
    CS->CTL1 |= CS_CTL1_SELA__REFOCLK |
            CS_CTL1_SELB;                   // Source REFO to ACLK & BCLK
    CS->CTL2 &= ~(CS_CTL2_LFXTDRIVE_MASK);  // Configure to lowest drive-strength
    CS->KEY = 0;

    // Turn off PSS high-side supervisors
    PSS->KEY = PSS_KEY_KEY_VAL;
    PSS->CTL0 |= PSS_CTL0_SVSMHOFF;
    PSS->KEY = 0;

    // Enable PCM rude mode, which allows to device to enter LPM3 without
    // waiting for peripherals
    PCM->CTL1 = PCM_CTL0_KEY_VAL | PCM_CTL1_FORCE_LPM_ENTRY;

    // Enable retention for blocks at top (stack) and bottom (block0, which is always on)
    // (8blks/bank x 4banks=32 blocks)
    SYSCTL_A->SRAM_BLKRET_CTL0 = 0x80000001;

    // Enable global interrupt
    __enable_irq();
    while(1){
        SCB->SCR |= SCB_SCR_SLEEPONEXIT_Msk;    // Do not wake up on exit from ISR

        // Setting the sleep deep bit
        SCB->SCR |= (SCB_SCR_SLEEPDEEP_Msk);
        // Go to LPM3
        __sleep();
        __no_operation();                   // For debugger
    }
}

// Port1 ISR
void PORT1_IRQHandler(void)
{
    volatile uint32_t i;

    // Toggling the output on the LED
    if(P1->IFG & BIT1)
        P1->OUT ^= BIT0;

    // Delay for switch debounce
    for(i = 0; i < 1000; i++);

    // Clear the interrupt flag
    P1->IFG &= ~BIT1;
}
