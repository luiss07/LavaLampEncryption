#include "lavaLampInterruptsAndSetup.h"

int joystickButtonPressed = 0;
int rightUpButtonPressed = 0;
int rightDownButtonPressed = 0;

uint16_t moveUp = 0;
uint16_t moveDown = 0;
uint16_t moveLeft = 0;
uint16_t moveRight = 0;

/* ADC results buffer */
static uint16_t resultsBuffer[2];

uint8_t TXData = 0;
uint8_t RXData = 0;

uint8_t rxBuf[RX_BUF_SIZE];
volatile int rxIndex = 0;
bool finished = false;

/* UART Configuration Parameter. These are the configuration parameters to
 * make the eUSCI A UART module to operate with a 115200 baud rate. These
 * values were calculated using the online calculator that TI provides
 * at:
 * http://software-dl.ti.com/msp430/msp430_public_sw/mcu/msp430/MSP430BaudRateConverter/index.html
 */
const eUSCI_UART_ConfigV1 uartConfig =
{
        EUSCI_A_UART_CLOCKSOURCE_SMCLK,          // SMCLK Clock Source
        26,                                      // BRDIV = 26
        0,                                       // UCxBRF = 0
        111,                                      // UCxBRS = 111
        EUSCI_A_UART_NO_PARITY,                  // No Parity
        EUSCI_A_UART_LSB_FIRST,                  // changed from MSB_FIRST
        EUSCI_A_UART_ONE_STOP_BIT,               // One stop bit
        EUSCI_A_UART_MODE,                       // UART mode
        EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION,  // Oversampling
        EUSCI_A_UART_8_BIT_LEN                  // 8 bit data length
};

//----------------------------------------------------------------------
//----------------------INITIALIZATION FUNCTIONS------------------------
//----------------------------------------------------------------------
void _adcInit()
{
        /* Configures Pin 6.0 and 4.4 as ADC input */
        GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P6, GPIO_PIN0, GPIO_TERTIARY_MODULE_FUNCTION);
        GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN4, GPIO_TERTIARY_MODULE_FUNCTION);

        /* Initializing ADC (ADCOSC/64/8) */
        ADC14_enableModule();
        ADC14_initModule(ADC_CLOCKSOURCE_ADCOSC, ADC_PREDIVIDER_64, ADC_DIVIDER_8, 0);

        /* Configuring ADC Memory (ADC_MEM0 - ADC_MEM1 (A15, A9)  with repeat)
             * with internal 2.5v reference */
        ADC14_configureMultiSequenceMode(ADC_MEM0, ADC_MEM1, true);
        ADC14_configureConversionMemory(ADC_MEM0,
                ADC_VREFPOS_AVCC_VREFNEG_VSS,
                ADC_INPUT_A15, ADC_NONDIFFERENTIAL_INPUTS);

        ADC14_configureConversionMemory(ADC_MEM1,
                ADC_VREFPOS_AVCC_VREFNEG_VSS,
                ADC_INPUT_A9, ADC_NONDIFFERENTIAL_INPUTS);

        /* Enabling the interrupt when a conversion on channel 1 (end of sequence)
         *  is complete and enabling conversions */
        ADC14_enableInterrupt(ADC_INT1);

        /* Enabling Interrupts */
        Interrupt_enableInterrupt(INT_ADC14);
        Interrupt_enableMaster();

        /* Setting up the sample timer to automatically step through the sequence
         * convert.
         */
        ADC14_enableSampleTimer(ADC_AUTOMATIC_ITERATION);

        /* Triggering the start of the sample */
        ADC14_enableConversion();
        ADC14_toggleConversionTrigger();
}

void _gpioInit()
{
    // Configure right-up button (P5.1) as input
    GPIO_setAsInputPin(GPIO_PORT_P5, GPIO_PIN1);

    // Configure rising edge interrupt for right-up button
    GPIO_interruptEdgeSelect(GPIO_PORT_P5, GPIO_PIN1, GPIO_HIGH_TO_LOW_TRANSITION);
    GPIO_clearInterruptFlag(GPIO_PORT_P5, GPIO_PIN1);
    GPIO_enableInterrupt(GPIO_PORT_P5, GPIO_PIN1);

    // Configure right-down button (P3.5) as input
    GPIO_setAsInputPin(GPIO_PORT_P3, GPIO_PIN5);

    // Configure rising edge interrupt for right-down button
    GPIO_interruptEdgeSelect(GPIO_PORT_P3, GPIO_PIN5, GPIO_HIGH_TO_LOW_TRANSITION);
    GPIO_clearInterruptFlag(GPIO_PORT_P3, GPIO_PIN5);
    GPIO_enableInterrupt(GPIO_PORT_P3, GPIO_PIN5);

    // Enable interrupt for Port 5 and Port 3
    Interrupt_enableInterrupt(INT_PORT5);
    Interrupt_enableInterrupt(INT_PORT3);
}

void _graphicsInit()
{
    /* Initializes display */
    Crystalfontz128x128_Init();

    /* Set default screen orientation */
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);

    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128, &g_sCrystalfontz128x128_funcs);

    Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);

    Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);

    Graphics_setFont(&g_sContext, &g_sFontCmss14b);

    Graphics_clearDisplay(&g_sContext);
}

void _UARTInit()
{
    
    /* Selecting P3.2 and P3.3 in UART mode */
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P3,
             GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);
    
    /* Configuring UART Module */
    UART_initModule(EUSCI_A2_BASE, &uartConfig); // Configuring UART module with EUSCI_A2_BASE and custom config: baud rate 112500 with 24MHz clock rate
    UART_enableModule(EUSCI_A2_BASE); // Enable wart module with EUSCI_A2_BASE initialized before

    /* Enabling interrupt */
    UART_enableInterrupt(EUSCI_A2_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT); // enable UART-related interrupt sources to receive and transmit
    Interrupt_enableInterrupt(INT_EUSCIA2); // INT_EUSCIA2 is enabled in the interrupt controller (system-wide)
}

void _hwInit()
{
    /* Halting WDT and disabling master interrupts */
    WDT_A_holdTimer();
    Interrupt_disableMaster();

    /* Set the core voltage level to VCORE1 */
    PCM_setCoreVoltageLevel(PCM_VCORE1);

    /* Set 2 flash wait states for Flash bank 0 and 1*/
    FlashCtl_setWaitState(FLASH_BANK0, 2);
    FlashCtl_setWaitState(FLASH_BANK1, 2);

    /* Initializes Clock System */
    CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_48);
    CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_HSMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    // Initialize and start a one-shot timer
    Timer32_initModule(TIMER32_0_BASE, TIMER32_PRESCALER_1, TIMER32_32BIT, TIMER32_PERIODIC_MODE);

    _adcInit();
    _gpioInit();
    _graphicsInit();
    _UARTInit();
}

//----------------------------------------------------------------------
//----------------------INTERRUPT FUNCTIONS-----------------------------
//----------------------------------------------------------------------

/* This interrupt is fired whenever a conversion is completed and placed in
 * ADC_MEM1. This signals the end of conversion and the results array is
 * grabbed and placed in resultsBuffer */
void ADC14_IRQHandler(void)
{
    uint64_t status;

    status = ADC14_getEnabledInterruptStatus();
    ADC14_clearInterruptFlag(status);

    /* ADC_MEM1 conversion completed */
    if(status & ADC_INT1)
    {
        //puts("In ADC handler.\n");
        // reset the movement variables
        moveUp = 0;
        moveDown = 0;
        moveLeft = 0;
        moveRight = 0;
        /* Store ADC14 conversion results */
        resultsBuffer[0] = ADC14_getResult(ADC_MEM0);
        resultsBuffer[1] = ADC14_getResult(ADC_MEM1);

        if (resultsBuffer[0] > 14000 && resultsBuffer[1] < 10000 && resultsBuffer[1] > 6000) {
            moveRight = 1;
        }
        else if (resultsBuffer[0] < 2000 && resultsBuffer[1] < 10000 && resultsBuffer[1] > 6000) {
            moveLeft = 1;
        }
        else if (resultsBuffer[1] > 14000 && resultsBuffer[0] < 10000 && resultsBuffer[0] > 6000) {
            moveUp = 1;
        }
        else if (resultsBuffer[1] < 2000 && resultsBuffer[0] < 10000 && resultsBuffer[0] > 6000) {
            moveDown = 1;
        }

        /* Determine if JoyStick button is pressed */
        joystickButtonPressed = 0;
        if (!(P4IN & GPIO_PIN1))
            joystickButtonPressed = 1;
    }
}

void PORT5_IRQHandler(void)
{
    //rightUpButtonPressed = 0;
    __disable_irq();
    if (P5->IFG & BIT1){
        rightUpButtonPressed = 1;
        P5->IFG &= ~BIT1;
        //puts("Button Up pressed.\n");
    }
    __enable_irq();
}

void PORT3_IRQHandler(void)
{
    //rightDownButtonPressed = 0;
    __disable_irq();
    if (P3->IFG & BIT5){
        rightDownButtonPressed = 1;
        P3->IFG &= ~BIT5;
        //puts("Button Down pressed.\n");
    }
    __enable_irq();
}

//-----------------------------------------------------

/* EUSCI A2 UART ISR */
void EUSCIA2_IRQHandler(void)
{
    uint32_t status = UART_getEnabledInterruptStatus(EUSCI_A2_BASE);

    if(status & EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG)
    {
        rxBuf[rxIndex++] = UART_receiveData(EUSCI_A2_BASE);

        // Check if buffer is full
        if (rxIndex == RX_BUF_SIZE) {
            // Disable UART receive interrupt to avoid overlapping more interrupt
            UART_disableInterrupt(EUSCI_A2_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);

            // Do something with the received data
            int i;
            for (i = 0; i < RX_BUF_SIZE; i++) {
                printf("%d ",rxBuf[i]);
                // TODO: put hashedNumber as a private member of a class
                hashedNumber[i] = rxBuf[i];
            }
            printf("\n--------------\n");
            // Reset buffer index
            rxIndex = 0;
            // Enable UART receive interrupt
            UART_enableInterrupt(EUSCI_A2_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
        }
    }

}