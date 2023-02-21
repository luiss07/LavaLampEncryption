#include "lavaLampInterface.h"

/* Graphic library context */
Graphics_Context g_sContext;

/* ADC results buffer */
static uint16_t resultsBuffer[2];

int joystickButtonPressed;
int rightUpButtonPressed = 0;
int rightDownButtonPressed = 0;
// currentThemeCounter saves the current selected theme
int currentThemeCounter = 0;
char * themes[MAX_THEMES] = {"Dark mode", "Light mode", "Fire"};

uint16_t moveUp = 0;
uint16_t moveDown = 0;
uint16_t moveLeft = 0;
uint16_t moveRight = 0;

uint32_t FOREGROUND_COLOR = GRAPHICS_COLOR_LIME;
uint32_t BACKGROUND_COLOR = GRAPHICS_COLOR_BLACK;
uint32_t SELECTION_COLOR = GRAPHICS_COLOR_DARK_GRAY;

bool loadAnimation_user = true;  


uint8_t hashedNumber[32] = {164,189,205,253,192,177,250,155,255,112,152,127,127,111,114,75,34,72,234,87,90,23,222,123,234,65,162,1,2,3,10,8};


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


// Function to restart 2 second timer
void restartTimer2sec() 
{
    Timer32_setCount(TIMER32_0_BASE, 60000000); // 2 seconds (default was 6000000)
    Timer32_startTimer(TIMER32_0_BASE, true);
}

// Function to restart 1 second timer
void restartTimer500ms() 
{
    Timer32_setCount(TIMER32_0_BASE, 15000000); // 1 second
    Timer32_startTimer(TIMER32_0_BASE, true);
}

// Function to restart 0.2 second timer
void restartTimer200ms() 
{
    Timer32_setCount(TIMER32_0_BASE, 6000000); // 0.2 seconds
    Timer32_startTimer(TIMER32_0_BASE, true);
}

// Function to restart 0.05 second timer
void restartTimer50ms() 
{
    Timer32_setCount(TIMER32_0_BASE, 1500000); // 0.05 seconds
    Timer32_startTimer(TIMER32_0_BASE, true);
}

// Function to check if timer is expired
bool isTimerExpired() 
{
    return (Timer32_getValue(TIMER32_0_BASE) == 0);
}

void convert_number_to_result(int hashedNumber_len, char *result_as_string) 
{
    int i=0;
    for (; i<hashedNumber_len; i++) {
        // define a buffer
        char current_number_to_string[DIGITS_OF_HASHED_NUMBER_ARRAY];

        // save in current_number_to_string the value of the current number in the array
        sprintf(current_number_to_string, "%d", hashedNumber[i]);

        strcat(result_as_string, current_number_to_string);
    }
}

// function to strip a string from its first N chars.
// source: https://stackoverflow.com/questions/4761764/how-to-remove-first-three-characters-from-string-with-c
void chopN(char *str, size_t n) 
{
    size_t len = strlen(str);
    if (n > len)
        return;  // Or: n = len;
    memmove(str, str+n, len - n + 1);
}

void drawIntegerPage() 
{
    Graphics_clearDisplay(&g_sContext);
    // at 14 pixels per character height, 19 numbers can be printed in a single line
    // the hashed number will have at most 32*3 = 96 digits
    //char * result_as_string_old[] = {"164189205253192177", "250547562112152027", "722317114820610423", "711983414318222821", "610464162123108"};
    //char * result_as_string = "164189205253192177250547562112152027722317114820610423711983414318222821610464162123108";

    char result_as_string[ROWS_FOR_HASH_NUMBER*MAX_DIGITS_INLINE] = "";
    //char result_as_string[ROWS_FOR_HASH_NUMBER][MAX_DIGITS_INLINE];
    convert_number_to_result(32, result_as_string);

    Graphics_setFont(&g_sContext, &g_sFontCmss12);
    Graphics_drawString(&g_sContext, "The calculated seed is:", -1, 0, 0, true);

    Graphics_setFont(&g_sContext, &g_sFontCmss14b);
    int i = 0;
    for (; i<ROWS_FOR_HASH_NUMBER; i++){
        Graphics_drawString(&g_sContext, result_as_string, MAX_DIGITS_INLINE, 0, 14*(i+2), false);
        chopN(result_as_string, MAX_DIGITS_INLINE-1);
        if (loadAnimation_user) restartTimer500ms();
        while(!isTimerExpired()); // Wait 0.2 seconds
    }

    
    int currentHoveredSelection = 0, offsetY = 31;
    int offsetsY[2] = {79, 79};
    int offsetsX[2] = {0, 90};

    // array to remember which selection we are hovering on
    bool hoveredSelection[2] = {true, false};
    bool selectionChanged = false;

    // array of the different menu selections
    char * menuSelections[2] = {"Get new seed", "Go back"};

    Graphics_setFont(&g_sContext, &g_sFontCmss12);

    // draw the menu selections
    while(1) {
        int i=0;
        for(; i<2; i++) {
            if (hoveredSelection[i]) {
                Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
                Graphics_setBackgroundColor(&g_sContext, SELECTION_COLOR);
                Graphics_drawString(&g_sContext, menuSelections[i], -1, offsetsX[i], offsetY + offsetsY[i], true);
                Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
                Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);
            }
            else {
                Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
                Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);
                Graphics_drawString(&g_sContext, menuSelections[i], -1, offsetsX[i], offsetY + offsetsY[i], true);
            }
            if (loadAnimation_user && !selectionChanged) restartTimer200ms();
            while(!isTimerExpired()); // Wait 0.2 seconds
        }
        restartTimer200ms();
        while(!isTimerExpired()); // Wait 0.2 seconds
        while (moveLeft == 0 && moveRight == 0 && joystickButtonPressed == 0) {};
        selectionChanged = true;
        if (moveLeft == 1) {
            if (currentHoveredSelection == 0) {
                hoveredSelection[currentHoveredSelection] = false;
                currentHoveredSelection = 2 - 1;
                hoveredSelection[currentHoveredSelection] = true;
            }
            else {
                hoveredSelection[currentHoveredSelection] = false;
                currentHoveredSelection -= 1;
                hoveredSelection[currentHoveredSelection] = true;
            }
        }
        else if (moveRight == 1) {
            if (currentHoveredSelection == 2-1) {
                hoveredSelection[currentHoveredSelection] = false;
                currentHoveredSelection = 0;
                hoveredSelection[currentHoveredSelection] = true;
            }
            else {
                hoveredSelection[currentHoveredSelection] = false;
                currentHoveredSelection += 1;
                hoveredSelection[currentHoveredSelection] = true;
            }
        }
        else if (joystickButtonPressed == 1) {
            if (strcmp(menuSelections[currentHoveredSelection], "Get new seed") == 0) {
                
                //TODO: send the ESP a generate new seed request

                //TODO: maybe wait some time before refreshing the page
                if (loadAnimation_user) transitionPage();
                drawIntegerPage();
            }
            else if (strcmp(menuSelections[currentHoveredSelection], "Go back") == 0) {
                if (loadAnimation_user) transitionPage();
                mainMenuPage();
            }
        }
        restartTimer200ms();
        while(!isTimerExpired()); // Wait 0.2 seconds
    };
}

void aboutPage() 
{
    Graphics_clearDisplay(&g_sContext);
    Graphics_setFont(&g_sContext, &g_sFontCmss12);

    Graphics_drawString(&g_sContext, "LavaLamp Encrypt V1.0", -1, 0, 0, true);
    Graphics_setFont(&g_sContext, &g_sFontCmss12);
    if (loadAnimation_user) restartTimer500ms();
    while(!isTimerExpired()); // Wait 0.2 seconds
    Graphics_drawString(&g_sContext, "Contributors:", -1, 0, 24, true);
    Graphics_setFont(&g_sContext, &g_sFontCmss12b);
    if (loadAnimation_user) restartTimer200ms();
    while(!isTimerExpired()); // Wait 0.2 seconds
    Graphics_drawString(&g_sContext, "Luigi Dell'Eva", -1, 0, 36, true);
    if (loadAnimation_user) restartTimer200ms();
    while(!isTimerExpired()); // Wait 0.2 seconds
    Graphics_drawString(&g_sContext, "Riccardo Germenia", -1, 0, 48, true);
    if (loadAnimation_user) restartTimer200ms();
    while(!isTimerExpired()); // Wait 0.2 seconds
    Graphics_drawString(&g_sContext, "Luca Cavalcanti", -1, 0, 60, true);

    Graphics_setForegroundColor(&g_sContext, SELECTION_COLOR);
    Graphics_Rectangle myRect = {42, 108, 85, 122};
    Graphics_fillRectangle(&g_sContext, &myRect);

    Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
    Graphics_setFont(&g_sContext, &g_sFontCmss12);
    if (loadAnimation_user) restartTimer500ms();
    while(!isTimerExpired()); // Wait 0.2 seconds
    Graphics_drawString(&g_sContext, "Go back", -1, 45, 110, false);

    Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
    Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);

    restartTimer200ms();
    while(!isTimerExpired()); // Wait 0.2 seconds

    while(joystickButtonPressed == 0) {};
    if (loadAnimation_user) transitionPage();
    mainMenuPage();
}

void generateRandomNumberPage() 
{
    Graphics_clearDisplay(&g_sContext);
    Graphics_setFont(&g_sContext, &g_sFontCmss12);

    Graphics_drawString(&g_sContext, "True Random Number", -1, 10, 0, true);
    Graphics_drawString(&g_sContext, "Generator", -1, 40, 12, true);

    Graphics_drawLineH(&g_sContext, 0, 128, 25);

    long long int min = 1, max = 100;
    int res;
    char min_to_string[MAX_RANDOM_NUMBER_DIGITS];
    char max_to_string[MAX_RANDOM_NUMBER_DIGITS];
    char res_to_string[MAX_RANDOM_NUMBER_DIGITS];
    sprintf(min_to_string, "%d", min);
    sprintf(max_to_string, "%d", max);

    //TODO: move this in the interrupt between ESP and MSP comms
    char conversion_result[ROWS_FOR_HASH_NUMBER*MAX_DIGITS_INLINE] = "";
    convert_number_to_result(32, conversion_result);

    char seed_from_hash[18] = "";

    strncpy(seed_from_hash, conversion_result, 18);

    srand(atoll(seed_from_hash));
    //----- until here 

    Graphics_drawString(&g_sContext, "Min: ", -1, 0, 31, true);
    Graphics_drawString(&g_sContext, min_to_string, -1, 30, 31, true);
    Graphics_drawString(&g_sContext, "Max: ", -1, 0, 43, true);
    Graphics_drawString(&g_sContext, max_to_string, -1, 30, 43, true);

    Graphics_drawString(&g_sContext, "GENERATE", -1, 0, 61, true);

    Graphics_drawString(&g_sContext, "Result: ", -1, 0, 79, true);
    //Graphics_drawString(&g_sContext, seed_from_hash, -1, 0, 79, true);

    Graphics_drawString(&g_sContext, "How to", -1, 0, 97, true);

    restartTimer200ms();
    while(!isTimerExpired()); // Wait 0.2 seconds

    int currentHoveredSelection = 0, offsetY = 31;
    int offsetsY[GENERATE_PAGE_SELECTIONS] = {0, 12, 30, 66, 79};
    int offsetsX[GENERATE_PAGE_SELECTIONS] = {30, 30, 0, 0, 45};

    // array to remember which selection we are hovering on
    bool hoveredSelection[GENERATE_PAGE_SELECTIONS] = {true, false, false, false, false};
    bool selectionChanged = false;

    // array of the different menu selections
    char * menuSelections[GENERATE_PAGE_SELECTIONS] = {min_to_string, max_to_string, "GENERATE", "How to", "Go back"};

    Graphics_setFont(&g_sContext, &g_sFontCmss12);

    // draw the menu selections
    while(1) {
        int i=0;
        for(; i<GENERATE_PAGE_SELECTIONS; i++) {
            if (hoveredSelection[i]) {
                Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
                Graphics_setBackgroundColor(&g_sContext, SELECTION_COLOR);
                Graphics_drawString(&g_sContext, menuSelections[i], -1, offsetsX[i], offsetY + offsetsY[i], true);
                Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
                Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);
            }
            else {
                Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
                Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);
                Graphics_drawString(&g_sContext, menuSelections[i], -1, offsetsX[i], offsetY + offsetsY[i], true);
            }
            if (loadAnimation_user && !selectionChanged) restartTimer200ms();
            while(!isTimerExpired()); // Wait 0.2 seconds
        }
        restartTimer200ms();
        while(!isTimerExpired()); // Wait 0.2 seconds
        while (moveUp == 0 && moveDown == 0 && moveLeft == 0 && moveRight == 0 && joystickButtonPressed == 0 && rightUpButtonPressed == 0 && rightDownButtonPressed == 0) {};
        selectionChanged = true;
        if (moveUp == 1) {
            if (currentHoveredSelection == 0) {
                hoveredSelection[currentHoveredSelection] = false;
                currentHoveredSelection = GENERATE_PAGE_SELECTIONS - 1;
                hoveredSelection[currentHoveredSelection] = true;
            }
            else {
                hoveredSelection[currentHoveredSelection] = false;
                currentHoveredSelection -= 1;
                hoveredSelection[currentHoveredSelection] = true;
            }
        }
        else if (moveDown == 1) {
            if (currentHoveredSelection == GENERATE_PAGE_SELECTIONS-1) {
                hoveredSelection[currentHoveredSelection] = false;
                currentHoveredSelection = 0;
                hoveredSelection[currentHoveredSelection] = true;
            }
            else {
                hoveredSelection[currentHoveredSelection] = false;
                currentHoveredSelection += 1;
                hoveredSelection[currentHoveredSelection] = true;
            }
        }
        else if (moveRight == 1 && strcmp(menuSelections[currentHoveredSelection], min_to_string) == 0) {
            Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
            Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);
            if (min < max-1 && min+1 <= 999999999) min += 1;
            sprintf(min_to_string, "%d", min);
            Graphics_drawString(&g_sContext, "                     ", -1, offsetsX[currentHoveredSelection], offsetY + offsetsY[currentHoveredSelection], true);
        }
        else if  (moveRight == 1 && strcmp(menuSelections[currentHoveredSelection], max_to_string) == 0) {
            Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
            Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);
            if (max+1 <= 999999999) max += 1;
            sprintf(max_to_string, "%d", max);
            Graphics_drawString(&g_sContext, "                     ", -1, offsetsX[currentHoveredSelection], offsetY + offsetsY[currentHoveredSelection], true);
        }
        else if (moveLeft == 1 && strcmp(menuSelections[currentHoveredSelection], min_to_string) == 0) {
            Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
            Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);
            if (min-1 >= -999999999) min -= 1;
            sprintf(min_to_string, "%d", min);
            Graphics_drawString(&g_sContext, "                     ", -1, offsetsX[currentHoveredSelection], offsetY + offsetsY[currentHoveredSelection], true);
        }
        else if  (moveLeft == 1 && strcmp(menuSelections[currentHoveredSelection], max_to_string) == 0) {
            Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
            Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);
            if (max > min+1 && max-1 >= -999999999) max -= 1;
            sprintf(max_to_string, "%d", max);
            Graphics_drawString(&g_sContext, "                     ", -1, offsetsX[currentHoveredSelection], offsetY + offsetsY[currentHoveredSelection], true);
        }
        else if (rightUpButtonPressed == 1 && strcmp(menuSelections[currentHoveredSelection], min_to_string) == 0) {
            rightUpButtonPressed = 0;
            // multiply min by 10
            Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
            Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);
            if (min*10 < max && min*10 <= 999999999 && min*10 >= -999999999) min = min*10;
            sprintf(min_to_string, "%d", min);
            Graphics_drawString(&g_sContext, "                     ", -1, offsetsX[currentHoveredSelection], offsetY + offsetsY[currentHoveredSelection], true);
        }
        else if (rightDownButtonPressed == 1 && strcmp(menuSelections[currentHoveredSelection], min_to_string) == 0) {
            rightDownButtonPressed = 0;
            // divide min by 10
            Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
            Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);
            if (min/10 < max && !(min/10 == 0 && max == 0)) min = min/10;
            sprintf(min_to_string, "%d", min);
            Graphics_drawString(&g_sContext, "                     ", -1, offsetsX[currentHoveredSelection], offsetY + offsetsY[currentHoveredSelection], true);
        }
        else if (rightUpButtonPressed == 1 && strcmp(menuSelections[currentHoveredSelection], max_to_string) == 0) {
            rightUpButtonPressed = 0;
            // multiply max by 10
            Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
            Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);
            if (max*10 <= 999999999 && max*10 >= -999999999) max = max*10;
            sprintf(max_to_string, "%d", max);
            Graphics_drawString(&g_sContext, "                     ", -1, offsetsX[currentHoveredSelection], offsetY + offsetsY[currentHoveredSelection], true);
        }
        else if (rightDownButtonPressed == 1 && strcmp(menuSelections[currentHoveredSelection], max_to_string) == 0) {
            rightDownButtonPressed = 0;
            // divide max by 10
            Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
            Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);
            if (max/10 > min && !(min == 0 && max/10 == 0)) max = max/10;
            sprintf(max_to_string, "%d", max);
            Graphics_drawString(&g_sContext, "                     ", -1, offsetsX[currentHoveredSelection], offsetY + offsetsY[currentHoveredSelection], true);
        }
        else if (joystickButtonPressed == 1) {
            if (strcmp(menuSelections[currentHoveredSelection], "How to") == 0) {
                if (loadAnimation_user) transitionPage();
                howToPage();
            }
            else if (strcmp(menuSelections[currentHoveredSelection], "GENERATE") == 0) {
                strcpy(res_to_string, "");
                res = rand() % max + min;
                sprintf(res_to_string, "%d", res);
                //delete any possible number there was first
                Graphics_drawString(&g_sContext, "                     ", -1, 35, 79, true);
                Graphics_drawString(&g_sContext, res_to_string, -1, 35, 79, true);
            }
            else if (strcmp(menuSelections[currentHoveredSelection], "Go back") == 0) {
                if (loadAnimation_user) transitionPage();
                mainMenuPage();
            }
        }
        restartTimer200ms();
        while(!isTimerExpired()); // Wait 0.2 seconds
    };
}

void howToPage() 
{
    Graphics_clearDisplay(&g_sContext);

    //TODO: fix here

    Graphics_setFont(&g_sContext, &g_sFontCmss12);
    Graphics_drawString(&g_sContext, "Usage: Increment or decre-", -1, 0, 0, false);
    Graphics_drawString(&g_sContext, "ment by 1 min and max by ", -1, 0, 12, false);
    Graphics_drawString(&g_sContext, "scrolling to the left/right", -1, 0, 24, false);
    Graphics_drawString(&g_sContext, "the joystick.", -1, 0, 36, false);
    Graphics_drawString(&g_sContext, "Use the right upper button", -1, 0, 48, false);
    Graphics_drawString(&g_sContext, "to multiply by 10.", -1, 0, 60, false);
    Graphics_drawString(&g_sContext, "Use the right lower button", -1, 0, 72, false);
    Graphics_drawString(&g_sContext, "to divide by 10.", -1, 0, 84, false);
    Graphics_drawString(&g_sContext, "Press on the GENERATE button", -1, 0, 96, false);
    Graphics_drawString(&g_sContext, "to generate the number", -1, 0, 108, false);
    Graphics_drawString(&g_sContext, "with the given boundaries.", -1, 0, 120, false);

    Graphics_setForegroundColor(&g_sContext, SELECTION_COLOR);
    Graphics_Rectangle myRect = {42, 108, 85, 122};
    Graphics_fillRectangle(&g_sContext, &myRect);

    Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
    Graphics_setFont(&g_sContext, &g_sFontCmss12);
    Graphics_drawString(&g_sContext, "Go back", -1, 45, 110, false);

    Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
    Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);

    restartTimer200ms();
    while(!isTimerExpired()); // Wait 0.2 seconds

    while(joystickButtonPressed == 0) {};
    if (loadAnimation_user) transitionPage();
    generateRandomNumberPage();
}

void changeTheme() 
{
    Graphics_clearDisplay(&g_sContext);
    if (strcmp(themes[currentThemeCounter], "Dark mode") == 0) {
        FOREGROUND_COLOR = GRAPHICS_COLOR_LIME;
        BACKGROUND_COLOR = GRAPHICS_COLOR_BLACK;
        SELECTION_COLOR = GRAPHICS_COLOR_DARK_GRAY;
    }
    else if (strcmp(themes[currentThemeCounter], "Light mode") == 0) {
        FOREGROUND_COLOR = GRAPHICS_COLOR_BLACK;
        BACKGROUND_COLOR = GRAPHICS_COLOR_WHITE;
        SELECTION_COLOR = GRAPHICS_COLOR_DARK_GRAY;
    }
    else if (strcmp(themes[currentThemeCounter], "Fire") == 0) {
        FOREGROUND_COLOR = 0x00FF0022;
        BACKGROUND_COLOR = 0x00A3A5C3;
        SELECTION_COLOR =  0x00F0A202;
    }
}

void settingsPage() 
{
    Graphics_clearDisplay(&g_sContext);
    Graphics_setFont(&g_sContext, &g_sFontCmss12);

    Graphics_drawString(&g_sContext, "Theme: ", -1, 0, 0, false);

    Graphics_drawString(&g_sContext, "Animations: ", -1, 0, 15, false);

    restartTimer200ms();
    while(!isTimerExpired()); // Wait 0.2 seconds

    int currentHoveredSelection = 0, offsetY = 0;

    char current_theme[64];
    strcpy(current_theme, themes[currentThemeCounter]);

    char animation_status[5];
    if(loadAnimation_user) {
        strcpy(animation_status, "ON");
    }
    else {
        strcpy(animation_status, "OFF");
    }

    int offsetsY[SETTINGS_PAGE_SELECTIONS] = {0, 15, 110};
    int offsetsX[SETTINGS_PAGE_SELECTIONS] = {40, 55, 20};
    // array to remember which selection we are hovering on
    bool hoveredSelection[SETTINGS_PAGE_SELECTIONS] = {true, false, false};
    bool selectionChanged = false;

    // array of the different menu selections
    char * menuSelections[SETTINGS_PAGE_SELECTIONS] = {current_theme, animation_status, "Go back and save"};

    Graphics_setFont(&g_sContext, &g_sFontCmss12);

    // draw the menu selections
    while(1) {
        Graphics_setFont(&g_sContext, &g_sFontCmss12);
        Graphics_drawString(&g_sContext, "Theme: ", -1, 0, 0, true);

        Graphics_drawString(&g_sContext, "Animations: ", -1, 0, 15, true);
        int i=0;
        for(; i<SETTINGS_PAGE_SELECTIONS; i++) {
            if (hoveredSelection[i]) {
                Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
                Graphics_setBackgroundColor(&g_sContext, SELECTION_COLOR);
                Graphics_drawString(&g_sContext, menuSelections[i], -1, offsetsX[i], offsetY + offsetsY[i], true);
                Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
                Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);
            }
            else {
                // Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
                // Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);
                Graphics_drawString(&g_sContext, menuSelections[i], -1, offsetsX[i], offsetY + offsetsY[i], true);
            }
        }
        restartTimer200ms();
        while(!isTimerExpired()); // Wait 0.2 seconds
        while (moveUp == 0 && moveDown == 0 && moveLeft == 0 && moveRight == 0 && joystickButtonPressed == 0) {};
        selectionChanged = true;
        if (moveUp == 1) {
            if (currentHoveredSelection == 0) {
                hoveredSelection[currentHoveredSelection] = false;
                currentHoveredSelection = SETTINGS_PAGE_SELECTIONS - 1;
                hoveredSelection[currentHoveredSelection] = true;
            }
            else {
                hoveredSelection[currentHoveredSelection] = false;
                currentHoveredSelection -= 1;
                hoveredSelection[currentHoveredSelection] = true;
            }
        }
        else if (moveDown == 1) {
            if (currentHoveredSelection == SETTINGS_PAGE_SELECTIONS-1) {
                hoveredSelection[currentHoveredSelection] = false;
                currentHoveredSelection = 0;
                hoveredSelection[currentHoveredSelection] = true;
            }
            else {
                hoveredSelection[currentHoveredSelection] = false;
                currentHoveredSelection += 1;
                hoveredSelection[currentHoveredSelection] = true;
            }
        }
        else if (moveLeft == 1 && strcmp(menuSelections[currentHoveredSelection], current_theme) == 0) {
            Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
            Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);
            if (currentThemeCounter == 0) currentThemeCounter = MAX_THEMES - 1;
            else currentThemeCounter--;
            strcpy(current_theme, themes[currentThemeCounter]);
            Graphics_drawString(&g_sContext, "                     ", -1, offsetsX[currentHoveredSelection], offsetY + offsetsY[currentHoveredSelection], true);
        }
        else if ((moveRight == 1 || joystickButtonPressed == 1) && strcmp(menuSelections[currentHoveredSelection], current_theme) == 0) {
            Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
            Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);
            if (currentThemeCounter == MAX_THEMES - 1) currentThemeCounter = 0;
            else currentThemeCounter++;
            strcpy(current_theme, themes[currentThemeCounter]);
            Graphics_drawString(&g_sContext, "                     ", -1, offsetsX[currentHoveredSelection], offsetY + offsetsY[currentHoveredSelection], true);
        }
        else if ((moveLeft == 1 || moveRight == 1 || joystickButtonPressed == 1) && strcmp(menuSelections[currentHoveredSelection], animation_status) == 0) {
            if (strcmp(menuSelections[currentHoveredSelection], "OFF") == 0) {
                Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
                Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);
                loadAnimation_user = true;
                strcpy(animation_status, "ON");
                Graphics_drawString(&g_sContext, "   ", -1, 55, 15, true);
            }
            else if (strcmp(menuSelections[currentHoveredSelection], "ON") == 0) {
                Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
                Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);
                loadAnimation_user = false;
                strcpy(animation_status, "OFF");
                Graphics_drawString(&g_sContext, "  ", -1, 55, 15, true);
            }
        }
        else if (joystickButtonPressed == 1) {
            if (strcmp(menuSelections[currentHoveredSelection], "Go back and save") == 0) {
                if (loadAnimation_user) transitionPage();
                mainMenuPage();
            }
        }
        restartTimer200ms();
        while(!isTimerExpired()); // Wait 0.2 seconds
        changeTheme();
    };
}

void transitionPage() 
{
    Graphics_setFont(&g_sContext, &g_sFontCmss14b);
    Graphics_clearDisplay(&g_sContext);
    char animated_matrix[30][19] = {
        "                   ",
        "                   ",
        "                   ",
        "                   ",
        "                   ",
        "                   ",
        "                   ",
        "                   ",
        "                   ",
        "                   ",
        "011 010110 01 001 1",
        "110 00101  11 100 1",
        " 01011100  01 010 0",
        " 1010 011 100101 11",
        " 0 00 111 011 00 01",
        "00 11 1 0 111 01 11",
        "11 10 0 01011 11 10",
        " 0 10 0 110 001 010",
        " 01 0101111 110 110",
        " 00 01001 0 111 111",
        "                   ",
        "                   ",
        "                   ",
        "                   ",
        "                   ",
        "                   ",
        "                   ",
        "                   ",
        "                   ",
        "                   ",
    };
    char empty_string[19] = "                   ";
    int i=0;
    for(; i<30; i++){
        int j=0;
        for(; j<10; j++) {
            if (i+j < 30) Graphics_drawString(&g_sContext, animated_matrix[29-(i+j)], -1, 0, 14*(9-j), true);
        }
        if (joystickButtonPressed || rightDownButtonPressed || rightUpButtonPressed) {
            if (rightDownButtonPressed) rightDownButtonPressed = 0;
            if (rightUpButtonPressed) rightUpButtonPressed = 0;
            break;  
        }
    }
    Graphics_setFont(&g_sContext, &g_sFontCmss12);
}

void mainMenuPage() 
{
    Graphics_clearDisplay(&g_sContext);
    Graphics_setFont(&g_sContext, &g_sFontCmss20b);

    char string_lavalamp[8] = "LavaLamp";
    int32_t lavaLampGradient[8] = {      0x00E80030,
                                        0x00EA4505,
                                        0x00FD6400,
                                        0x00FB6E03,
                                        0x00FDAD00,
                                        0x00FB7905,
                                        0x00A85B00,
                                        0x00A85B02,
    };

    char string_encrypt[11] = "Encrypt 1.0";
    int32_t encryptGradient[11] = {     0x00E5F9BA,
                                        0x00F4DAD3,
                                        0x00D5B4B9,
                                        0x00D2A780,
                                        0x00C28D69,
                                        0x00A5696B,
                                        0x0056365E,
                                        0x00000000,
                                        0x00E5F9BA,
                                        0x00D2A780,
                                        0x0056365E,
    };

    // value to keep track of Y offset
    int offsetY = 0;

    int i;
    for (i = 0; i < 8; i++) {
        Graphics_setForegroundColor(&g_sContext, lavaLampGradient[i]);
        if (i==7) {
            Graphics_drawString(&g_sContext, string_lavalamp, 1, 90, offsetY, true);
        }
        else Graphics_drawString(&g_sContext, string_lavalamp, 1, i*12, offsetY, true);
        chopN(string_lavalamp, 1);
        if (loadAnimation_user) restartTimer50ms();
        while(!isTimerExpired()); // Wait 2 seconds
    }

    offsetY += 20;

    int j;
    for (j = 0; j < 11; j++) {
        Graphics_setForegroundColor(&g_sContext, encryptGradient[j]);
        Graphics_drawString(&g_sContext, string_encrypt, 1, j*11, offsetY, true);
        chopN(string_encrypt, 1);
        if (loadAnimation_user) restartTimer50ms();
        while(!isTimerExpired()); // Wait 2 seconds
    }


    Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);

    Graphics_drawLineH(&g_sContext, 0, 128, 45);

    offsetY += 40;

    int currentHoveredSelection = 0;
    // array to remember which selection we are hovering on
    bool hoveredSelection[MAX_MENU_SELECTIONS] = {true, false, false, false};

    // array of the different menu selections
    char * menuSelections[MAX_MENU_SELECTIONS] = {"Generated Seed", "Generate Random Number", "About", "Settings"};

    Graphics_setFont(&g_sContext, &g_sFontCmss12);
    bool selectionChanged = false;

    // draw the menu selections
    while(1) {
        int i=0;
        for(; i<MAX_MENU_SELECTIONS; i++) {
            if (hoveredSelection[i]) {
                Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
                Graphics_setBackgroundColor(&g_sContext, SELECTION_COLOR);
                Graphics_drawString(&g_sContext, menuSelections[i], -1, 0, offsetY + 16*i, true);
                Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
                Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);
            }
            else {
                Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
                Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);
                Graphics_drawString(&g_sContext, menuSelections[i], -1, 0, offsetY + 16*i, true);
            }
            if (loadAnimation_user && !selectionChanged) restartTimer500ms();
            while(!isTimerExpired()); // Wait 0.2 seconds
        }
        restartTimer200ms();
        while(!isTimerExpired()); // Wait 0.2 seconds
        while (moveUp == 0 && moveDown == 0 && joystickButtonPressed == 0) {};
        selectionChanged = true;
        if (moveUp == 1) {
            if (currentHoveredSelection == 0) {
                hoveredSelection[currentHoveredSelection] = false;
                currentHoveredSelection = MAX_MENU_SELECTIONS - 1;
                hoveredSelection[currentHoveredSelection] = true;
            }
            else {
                hoveredSelection[currentHoveredSelection] = false;
                currentHoveredSelection -= 1;
                hoveredSelection[currentHoveredSelection] = true;
            }
        }
        else if (moveDown == 1) {
            if (currentHoveredSelection == MAX_MENU_SELECTIONS-1) {
                hoveredSelection[currentHoveredSelection] = false;
                currentHoveredSelection = 0;
                hoveredSelection[currentHoveredSelection] = true;
            }
            else {
                hoveredSelection[currentHoveredSelection] = false;
                currentHoveredSelection += 1;
                hoveredSelection[currentHoveredSelection] = true;
            }
        }
        else if (joystickButtonPressed == 1) {
            if (strcmp(menuSelections[currentHoveredSelection], "Generated Seed") == 0) {
                if (loadAnimation_user) transitionPage();
                drawIntegerPage();
            }
            else if (strcmp(menuSelections[currentHoveredSelection], "Generate Random Number") == 0) {
                if (loadAnimation_user) transitionPage();
                generateRandomNumberPage();
            }
            else if (strcmp(menuSelections[currentHoveredSelection], "About") == 0) {
                if (loadAnimation_user) transitionPage();
                aboutPage();
            }
            else if (strcmp(menuSelections[currentHoveredSelection], "Settings") == 0) {
                if (loadAnimation_user) transitionPage();
                settingsPage();
            }
        }
    };
}



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
        printf("In ADC handler.\n");
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
        printf("Button Up pressed.\n");
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
        printf("Button Down pressed.\n");
    }
    __enable_irq();
}
