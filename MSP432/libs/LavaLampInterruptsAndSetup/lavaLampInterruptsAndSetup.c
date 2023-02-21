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

    _graphicsInit();
    _adcInit();
    _gpioInit();
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
