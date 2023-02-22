#include "lavaLampInterface.h"

uint8_t hashedNumber[32] = {164,189,205,253,192,177,250,155,255,112,152,127,127,111,114,75,34,72,234,87,90,23,222,123,234,65,162,1,2,3,10,8};

Graphics_Context g_sContext;

uint32_t FOREGROUND_COLOR = GRAPHICS_COLOR_LIME;
uint32_t BACKGROUND_COLOR = GRAPHICS_COLOR_BLACK;
uint32_t SELECTION_COLOR = GRAPHICS_COLOR_DARK_GRAY;

// currentThemeCounter saves the current selected theme
int currentThemeCounter = 0;
char * themes[MAX_THEMES] = {"Dark mode", "Light mode", "Fire"};

bool loadAnimation_user = true;  

//----------------------------------------------------------------------
//--------------------------HELPER FUNCTIONS----------------------------
//----------------------------------------------------------------------

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

// Function that restarts the Timer32 with the given time to wait in milliseconds and waits for it to expire
void restartAndWaitTimerExpiration(int timeToWaitInMillisec)
{
    switch (timeToWaitInMillisec)
    {
    case 2000:
        restartTimer2sec();
        break;

    case 500:
        restartTimer500ms();
        break;

    case 200:
        restartTimer200ms();
        break;
        
    case 50:
        restartTimer50ms();
        break;
        
    default:
        break;
    }

    while(!isTimerExpired()); // Wait timeToWaitInMillisec 
}

// Function to setup a selectableMenuSelections with its standard values
void setSelectableMenuSections
(
    struct selectableMenuSelections* selectablemenuselection, 
    int numberOfSelections,
    int currentHoveredSelection,
    int offsetY,
    int *offsetsY,
    int *offsetsX,

    bool *hoveredSelection,
    bool selectionHasChangedOnce,

    char **menuSelections
)
{
    selectablemenuselection->numberOfSelections = numberOfSelections;
    selectablemenuselection->currentHoveredSelection = currentHoveredSelection;
    selectablemenuselection->offsetY = offsetY;
    selectablemenuselection->offsetsY = offsetsY;
    selectablemenuselection->offsetsX = offsetsX;
    selectablemenuselection->hoveredSelection = hoveredSelection;
    selectablemenuselection->selectionHasChangedOnce = selectionHasChangedOnce;
    selectablemenuselection->menuSelections = menuSelections;
}

// Function to convert hashedNumber (global variable) into a string passed by reference: result_as_string
void convert_number_to_result(int hashedNumber_len, char *result_as_string) 
{
    int i=0;
    for (; i<hashedNumber_len; i++) {
        // define a buffer
        char current_number_to_string[DIGITS_OF_HASHED_NUMBER_ARRAY];

        // save in current_number_to_string the value of the current number in the array
        sprintf(current_number_to_string, "%d", hashedNumber[i]);

        // concatenate the buffer to the final string
        strcat(result_as_string, current_number_to_string);
    }
}

// Function to strip a string from its first N chars.
// source: https://stackoverflow.com/questions/4761764/how-to-remove-first-three-characters-from-string-with-c
void chopN(char *str, size_t n) 
{
    size_t len = strlen(str);
    if (n > len)
        return;  // Or: n = len;
    memmove(str, str+n, len - n + 1);
}

// Function to draw all the selectable selections (as in, hoverable)
void drawSelectableMenuSelections(const struct selectableMenuSelections* selectablemenuselections)
{
    int i=0;
    for(; i<selectablemenuselections->numberOfSelections; i++) {
        if (selectablemenuselections->hoveredSelection[i]) {
            Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
            Graphics_setBackgroundColor(&g_sContext, SELECTION_COLOR);
            Graphics_drawString(&g_sContext, selectablemenuselections->menuSelections[i], -1, selectablemenuselections->offsetsX[i], selectablemenuselections->offsetY + selectablemenuselections->offsetsY[i], true);
            Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
            Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);
        }
        else {
            Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
            Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);
            Graphics_drawString(&g_sContext, selectablemenuselections->menuSelections[i], -1, selectablemenuselections->offsetsX[i], selectablemenuselections->offsetY + selectablemenuselections->offsetsY[i], true);
        }
        if (loadAnimation_user && !selectablemenuselections->selectionHasChangedOnce) restartAndWaitTimerExpiration(500);
    }
}

// Function to change which selection is currently hovered on
void changeSelectedMenuSelection(struct selectableMenuSelections* selectablemenuselections, uint16_t directionMovedForDecrease, uint16_t directionMovedForIncrease)
{
    if (directionMovedForDecrease) {
        if (selectablemenuselections->currentHoveredSelection == 0) {
            selectablemenuselections->hoveredSelection[selectablemenuselections->currentHoveredSelection] = false;
            selectablemenuselections->currentHoveredSelection = selectablemenuselections->numberOfSelections - 1;
            selectablemenuselections->hoveredSelection[selectablemenuselections->currentHoveredSelection] = true;
        }
        else {
            selectablemenuselections->hoveredSelection[selectablemenuselections->currentHoveredSelection] = false;
            selectablemenuselections->currentHoveredSelection -= 1;
            selectablemenuselections->hoveredSelection[selectablemenuselections->currentHoveredSelection] = true;
        }
    }
    else if (directionMovedForIncrease) {
        if (selectablemenuselections->currentHoveredSelection == selectablemenuselections->numberOfSelections-1) {
            selectablemenuselections->hoveredSelection[selectablemenuselections->currentHoveredSelection] = false;
            selectablemenuselections->currentHoveredSelection = 0;
            selectablemenuselections->hoveredSelection[selectablemenuselections->currentHoveredSelection] = true;
        }
        else {
            selectablemenuselections->hoveredSelection[selectablemenuselections->currentHoveredSelection] = false;
            selectablemenuselections->currentHoveredSelection += 1;
            selectablemenuselections->hoveredSelection[selectablemenuselections->currentHoveredSelection] = true;
        }
    }
}

// Function to clear any previous selectable selection given its offset and length
void clearPreviousSelectableSelection(int offsetX, int offsetY, const char* blankString)
{
    Graphics_setForegroundColor(&g_sContext, BACKGROUND_COLOR);
    Graphics_drawString(&g_sContext, "            ", -1, offsetX, offsetY, true);
    Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
}

// Function to warp to a different page
void changePage(char* currentHoveredSelection_toString)
{
    if (loadAnimation_user) transitionPage();
    // main menu page warps
    if (strcmp(currentHoveredSelection_toString, "Generated Seed") == 0) {
        drawIntegerPage();
    }
    else if (strcmp(currentHoveredSelection_toString, "Generate Random Number") == 0) {
        generateRandomNumberPage();
    }
    else if (strcmp(currentHoveredSelection_toString, "About") == 0) {
        aboutPage();
    }
    else if (strcmp(currentHoveredSelection_toString, "Settings") == 0) {
        settingsPage();
    }
    // draw integer page warps
    else if (strcmp(currentHoveredSelection_toString, "Get new seed") == 0) {
        char* requestBody[12] = "GetPhoto\n";
        UART_transmitData(EUSCI_A2_BASE, requestBody);
        
        drawIntegerPage();
    }
    else if (strcmp(currentHoveredSelection_toString, "Go back") == 0) {
        mainMenuPage();
    }
    // settings page warps
    else if (strcmp(currentHoveredSelection_toString, "Go back and save") == 0) {
        mainMenuPage();
    }
    // generate random number page warps
    else if (strcmp(currentHoveredSelection_toString, "How to") == 0) {
        howToPage();
    }
}

//----------------------------------------------------------------------
//----------------------DRAW PAGES FUNCTIONS----------------------------
//----------------------------------------------------------------------
void drawIntegerPage() 
{
    Graphics_clearDisplay(&g_sContext);

    char result_as_string[ROWS_FOR_HASH_NUMBER*MAX_DIGITS_INLINE] = "";
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

    int tempOffsetsY[2] = {79, 79};
    int tempOffsetsX[2] = {0, 90};

    bool tempHoveredSelection[2] = {true, false};

    char * tempMenuSelections[2] = {"Get new seed", "Go back"};

    struct selectableMenuSelections selectableDrawIntegerSelections;
    setSelectableMenuSections(&selectableDrawIntegerSelections, 2, 0, 31, tempOffsetsY, tempOffsetsX, tempHoveredSelection, false, tempMenuSelections);

    Graphics_setFont(&g_sContext, &g_sFontCmss12);

    char *currentHoveredSelection_toString;
    
    while(1) {
        drawSelectableMenuSelections(&selectableDrawIntegerSelections);

        while (!moveLeft && !moveRight && !joystickButtonPressed) {};

        selectableDrawIntegerSelections.selectionHasChangedOnce = true;
        currentHoveredSelection_toString = selectableDrawIntegerSelections.menuSelections[selectableDrawIntegerSelections.currentHoveredSelection];
        
        if (moveLeft || moveRight) {
            changeSelectedMenuSelection(&selectableDrawIntegerSelections, moveLeft, moveRight);
        }
        else if (joystickButtonPressed == 1) {
            changePage(currentHoveredSelection_toString);
        }
        restartAndWaitTimerExpiration(200);
    };
}

void aboutPage() 
{
    Graphics_clearDisplay(&g_sContext);
    Graphics_setFont(&g_sContext, &g_sFontCmss12);

    Graphics_drawString(&g_sContext, "LavaLamp Encrypt V1.0", -1, 0, 0, true);

    Graphics_setFont(&g_sContext, &g_sFontCmss12);
    if (loadAnimation_user) restartAndWaitTimerExpiration(500);
    Graphics_drawString(&g_sContext, "Contributors:", -1, 0, 24, true);

    Graphics_setFont(&g_sContext, &g_sFontCmss12b);
    if (loadAnimation_user) restartAndWaitTimerExpiration(200);
    Graphics_drawString(&g_sContext, "Luigi Dell'Eva", -1, 0, 36, true);
    if (loadAnimation_user) restartAndWaitTimerExpiration(200);
    Graphics_drawString(&g_sContext, "Riccardo Germenia", -1, 0, 48, true);
    if (loadAnimation_user) restartAndWaitTimerExpiration(200);
    Graphics_drawString(&g_sContext, "Luca Cavalcanti", -1, 0, 60, true);

    Graphics_setForegroundColor(&g_sContext, SELECTION_COLOR);
    Graphics_Rectangle myRect = {42, 108, 85, 122};
    Graphics_fillRectangle(&g_sContext, &myRect);

    Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
    Graphics_setFont(&g_sContext, &g_sFontCmss12);
    if (loadAnimation_user) restartAndWaitTimerExpiration(500);
    Graphics_drawString(&g_sContext, "Go back", -1, 45, 110, false);

    Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
    Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);

    restartAndWaitTimerExpiration(200);

    while(!joystickButtonPressed) {};
    if (loadAnimation_user) transitionPage();
    mainMenuPage();
}

void generateRandomNumberPage() 
{
    drawGeneratePageStaticValues();

    //TODO: move this in the interrupt between ESP and MSP comms
    char conversion_result[ROWS_FOR_HASH_NUMBER*MAX_DIGITS_INLINE] = "";
    convert_number_to_result(32, conversion_result);

    char seed_from_hash[18] = "";

    strncpy(seed_from_hash, conversion_result, 18);

    srand(atoll(seed_from_hash));
    //----- until here 

    //Graphics_drawString(&g_sContext, "How to", -1, 0, 97, true);
    // int tempOffsetsY[GENERATE_PAGE_SELECTIONS] = {0, 12, 30, 66, 79};
    // int tempOffsetsX[GENERATE_PAGE_SELECTIONS] = {30, 30, 0, 0, 45};
    // bool tempHoveredSelection[GENERATE_PAGE_SELECTIONS] = {true, false, false, false, false};
    //char * tempMenuSelections[GENERATE_PAGE_SELECTIONS] = {selectableGeneratePageSelections.min_to_string, selectableGeneratePageSelections.max_to_string, "GENERATE", "How to", "Go back"};
    
    int tempOffsetsY[GENERATE_PAGE_SELECTIONS] = {0, 16, 34, 79};
    int tempOffsetsX[GENERATE_PAGE_SELECTIONS] = {30, 30, 0, 45};

    bool tempHoveredSelection[GENERATE_PAGE_SELECTIONS] = {true, false, false, false};

    struct selectableMenuSelections selectableGeneratePageSelections;

    selectableGeneratePageSelections.min = 1;
    selectableGeneratePageSelections.max = 100;

    char min_to_string[MAX_RANDOM_NUMBER_DIGITS];
    char max_to_string[MAX_RANDOM_NUMBER_DIGITS];
    sprintf(min_to_string, "%d", selectableGeneratePageSelections.min);
    sprintf(max_to_string, "%d", selectableGeneratePageSelections.max);

    selectableGeneratePageSelections.min_to_string = min_to_string;
    selectableGeneratePageSelections.max_to_string = max_to_string;

    char * tempMenuSelections[GENERATE_PAGE_SELECTIONS] = {selectableGeneratePageSelections.min_to_string, selectableGeneratePageSelections.max_to_string, "GENERATE", "Go back"};

    setSelectableMenuSections(&selectableGeneratePageSelections, GENERATE_PAGE_SELECTIONS, 0, 31, tempOffsetsY, tempOffsetsX, tempHoveredSelection, false, tempMenuSelections);

    Graphics_setFont(&g_sContext, &g_sFontCmss12);

    char *currentHoveredSelection_toString;

    int res;
    char res_to_string[MAX_RANDOM_NUMBER_DIGITS];

    while(1) {
        drawSelectableMenuSelections(&selectableGeneratePageSelections);

        while (!moveUp && !moveDown && !moveLeft && !moveRight && !joystickButtonPressed && !rightUpButtonPressed && !rightDownButtonPressed) {};

        selectableGeneratePageSelections.selectionHasChangedOnce = true;
        currentHoveredSelection_toString = selectableGeneratePageSelections.menuSelections[selectableGeneratePageSelections.currentHoveredSelection];
        if (moveUp || moveDown) {
            changeSelectedMenuSelection(&selectableGeneratePageSelections, moveUp, moveDown);
        }
        else if (moveRight || moveLeft || rightUpButtonPressed || rightDownButtonPressed) {
            changeMaxAndMinValues(&selectableGeneratePageSelections, currentHoveredSelection_toString);
        }
        else if (joystickButtonPressed) {
            if (strcmp(currentHoveredSelection_toString, "GENERATE") == 0) {
                strcpy(res_to_string, "");
                res = rand() % selectableGeneratePageSelections.max + selectableGeneratePageSelections.min;
                sprintf(res_to_string, "%d", res);
                clearPreviousSelectableSelection(35, 83, "                ");
                Graphics_drawString(&g_sContext, res_to_string, -1, 35, 83, true);
            }
            else {
                changePage(currentHoveredSelection_toString);
            }
        }
        restartAndWaitTimerExpiration(200);
    };
}

void drawGeneratePageStaticValues()
{
    
    Graphics_clearDisplay(&g_sContext);
    Graphics_setFont(&g_sContext, &g_sFontCmss12);

    Graphics_drawString(&g_sContext, "True Random Number", -1, 10, 0, true);
    Graphics_drawString(&g_sContext, "Generator", -1, 40, 12, true);

    Graphics_drawLineH(&g_sContext, 0, 128, 25);

    Graphics_drawString(&g_sContext, "Min: ", -1, 0, 31, true);
    Graphics_drawString(&g_sContext, "Max: ", -1, 0, 47, true);
    Graphics_drawString(&g_sContext, "Result: ", -1, 0, 83, true);

    restartAndWaitTimerExpiration(200);
}

void changeMaxAndMinValues(struct selectableMenuSelections* selectableGeneratePageSelections, char* currentHoveredSelection_toString)
{
    char* min_to_string = selectableGeneratePageSelections->min_to_string;
    char* max_to_string = selectableGeneratePageSelections->max_to_string;
    long long int min = selectableGeneratePageSelections->min;
    long long int max = selectableGeneratePageSelections->max;

    int offsetX = selectableGeneratePageSelections->offsetsX[selectableGeneratePageSelections->currentHoveredSelection];
    int offsetY = selectableGeneratePageSelections->offsetsY[selectableGeneratePageSelections->currentHoveredSelection] + selectableGeneratePageSelections->offsetY;

    if (moveRight && strcmp(currentHoveredSelection_toString, min_to_string) == 0) {
        if (min < max-1 && min+1 <= 999999999) min += 1;
        sprintf(min_to_string, "%d", min);
        clearPreviousSelectableSelection(offsetX, offsetY, "                     ");
    }
    else if (moveRight && strcmp(currentHoveredSelection_toString, max_to_string) == 0) {
        if (max+1 <= 999999999) max += 1;
        sprintf(max_to_string, "%d", max);
        clearPreviousSelectableSelection(offsetX, offsetY, "                     ");
    }
    else if (moveLeft && strcmp(currentHoveredSelection_toString, min_to_string) == 0) {
        if (min-1 >= -999999999) min -= 1;
        sprintf(min_to_string, "%d", min);
        clearPreviousSelectableSelection(offsetX, offsetY, "                     ");
    }
    else if  (moveLeft && strcmp(currentHoveredSelection_toString, max_to_string) == 0) {
        if (max > min+1 && max-1 >= -999999999) max -= 1;
        sprintf(max_to_string, "%d", max);
        clearPreviousSelectableSelection(offsetX, offsetY, "                     ");
    }
    else if (rightUpButtonPressed && strcmp(currentHoveredSelection_toString, min_to_string) == 0) {
        rightUpButtonPressed = 0;
        // multiply min by 10
        if (min*10 < max && min*10 <= 999999999 && min*10 >= -999999999) min = min*10;
        sprintf(min_to_string, "%d", min);
        clearPreviousSelectableSelection(offsetX, offsetY, "                     ");
    }
    else if (rightDownButtonPressed && strcmp(currentHoveredSelection_toString, min_to_string) == 0) {
        rightDownButtonPressed = 0;
        // divide min by 10
        if (min/10 < max && !(min/10 == 0 && max == 0)) min = min/10;
        sprintf(min_to_string, "%d", min);
        clearPreviousSelectableSelection(offsetX, offsetY, "                     ");
    }
    else if (rightUpButtonPressed && strcmp(currentHoveredSelection_toString, max_to_string) == 0) {
        rightUpButtonPressed = 0;
        // multiply max by 10
        if (max*10 <= 999999999 && max*10 >= -999999999) max = max*10;
        sprintf(max_to_string, "%d", max);
        clearPreviousSelectableSelection(offsetX, offsetY, "                     ");
    }
    else if (rightDownButtonPressed && strcmp(currentHoveredSelection_toString, max_to_string) == 0) {
        rightDownButtonPressed = 0;
        // divide max by 10
        if (max/10 > min && !(min == 0 && max/10 == 0)) max = max/10;
        sprintf(max_to_string, "%d", max);
        clearPreviousSelectableSelection(offsetX, offsetY, "                     ");
    }

    selectableGeneratePageSelections->min = min;
    selectableGeneratePageSelections->max = max;
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

    restartAndWaitTimerExpiration(200);

    while(joystickButtonPressed == 0) {};
    if (loadAnimation_user) transitionPage();
    generateRandomNumberPage();
}

void settingsPage() 
{
    drawSettingsPageStaticValues();

    char current_theme[64];
    strcpy(current_theme, themes[currentThemeCounter]);

    char animation_status[5];
    if(loadAnimation_user) {
        strcpy(animation_status, "ON");
    }
    else {
        strcpy(animation_status, "OFF");
    }

    int tempOffsetsY[SETTINGS_PAGE_SELECTIONS] = {0, 15, 110};
    int tempOffsetsX[SETTINGS_PAGE_SELECTIONS] = {40, 55, 20};

    bool tempHoveredSelection[SETTINGS_PAGE_SELECTIONS] = {true, false, false};

    char * tempMenuSelections[SETTINGS_PAGE_SELECTIONS] = {current_theme, animation_status, "Go back and save"};

    struct selectableMenuSelections selectableSettingsPageSelections;
    setSelectableMenuSections(&selectableSettingsPageSelections, SETTINGS_PAGE_SELECTIONS, 0, 0, tempOffsetsY, tempOffsetsX, tempHoveredSelection, false, tempMenuSelections);

    Graphics_setFont(&g_sContext, &g_sFontCmss12);

    char* currentHoveredSelection_toString;

    while(1) {
        Graphics_drawString(&g_sContext, "Theme: ", -1, 0, 0, true);
        Graphics_drawString(&g_sContext, "Animations: ", -1, 0, 15, true);
        drawSelectableMenuSelections(&selectableSettingsPageSelections);

        while (!moveUp && !moveDown && moveLeft && moveRight && joystickButtonPressed) {};

        selectableSettingsPageSelections.selectionHasChangedOnce = true;
        currentHoveredSelection_toString = selectableSettingsPageSelections.menuSelections[selectableSettingsPageSelections.currentHoveredSelection];

        int offsetX = selectableSettingsPageSelections.offsetsX[selectableSettingsPageSelections.currentHoveredSelection];
        int offsetY = selectableSettingsPageSelections.offsetsY[selectableSettingsPageSelections.currentHoveredSelection];
        
        if (moveUp || moveDown) {
            changeSelectedMenuSelection(&selectableSettingsPageSelections, moveUp, moveDown);
        }
        else if (strcmp(currentHoveredSelection_toString, current_theme) == 0) {
            checkForThemeChange(currentHoveredSelection_toString, current_theme, offsetX, offsetY);
        }
        else if ((moveLeft || moveRight || joystickButtonPressed) && strcmp(currentHoveredSelection_toString, animation_status) == 0) {
            checkForAnimationChange(currentHoveredSelection_toString, animation_status, offsetX, offsetY);
        }
        else if (joystickButtonPressed) {
            changePage(currentHoveredSelection_toString);
        }
        restartAndWaitTimerExpiration(500);
        changeTheme();
    };
}

void drawSettingsPageStaticValues()
{   
    Graphics_clearDisplay(&g_sContext);
    Graphics_setFont(&g_sContext, &g_sFontCmss12);

    Graphics_drawString(&g_sContext, "Theme: ", -1, 0, 0, false);

    Graphics_drawString(&g_sContext, "Animations: ", -1, 0, 15, false);

    restartAndWaitTimerExpiration(200);
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

bool checkForThemeChange(char* currentHoveredSelection_toString, char* current_theme, int offsetX, int offsetY)
{
    bool hasThemeChanged = false;
    if ((moveLeft == 1 || joystickButtonPressed == 1) && strcmp(currentHoveredSelection_toString, current_theme) == 0) {
            if (currentThemeCounter == 0) currentThemeCounter = MAX_THEMES - 1;
            else currentThemeCounter--;
            strcpy(current_theme, themes[currentThemeCounter]);
            clearPreviousSelectableSelection(offsetX, offsetY, "                     ");
            hasThemeChanged = true;
    }
    else if ((moveRight == 1 || joystickButtonPressed == 1) && strcmp(currentHoveredSelection_toString, current_theme) == 0) {
        if (currentThemeCounter == MAX_THEMES - 1) currentThemeCounter = 0;
        else currentThemeCounter++;
        strcpy(current_theme, themes[currentThemeCounter]);
        clearPreviousSelectableSelection(offsetX, offsetY, "                     ");
        hasThemeChanged = true;
    }

    return hasThemeChanged;
}

void checkForAnimationChange(char* currentHoveredSelection_toString, char* animation_status, int offsetX, int offsetY)
{
    if (strcmp(currentHoveredSelection_toString, "OFF") == 0) {
        Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
        Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);
        loadAnimation_user = true;
        strcpy(animation_status, "ON");
        clearPreviousSelectableSelection(offsetX, offsetY, "  ");
    }
    else if (strcmp(currentHoveredSelection_toString, "ON") == 0) {
        Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);
        Graphics_setBackgroundColor(&g_sContext, BACKGROUND_COLOR);
        loadAnimation_user = false;
        strcpy(animation_status, "OFF");
        clearPreviousSelectableSelection(offsetX, offsetY, "  ");
    }
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

    int offsetY = drawLavaLampTitleAndReturnOffsetY();

    offsetY += 40;

    // struct selectableMenuSelections selectableMainMenuSelections;
    // selectableMainMenuSelections = setMainMenuSelections(offsetY);

    struct selectableMenuSelections selectableMainMenuSelections;

    int tempOffsetsY[MAIN_MENU_SELECTIONS] = {0, 16, 32, 48};
    int tempOffsetsX[MAIN_MENU_SELECTIONS] = {2, 2, 2, 2};

    bool tempHoveredSelection[MAIN_MENU_SELECTIONS] = {true, false, false, false};

    char * tempMenuSelections[MAIN_MENU_SELECTIONS] = {"Generated Seed", "Generate Random Number", "About", "Settings"};

    setSelectableMenuSections(&selectableMainMenuSelections, MAIN_MENU_SELECTIONS, 0, offsetY, tempOffsetsY, tempOffsetsX, tempHoveredSelection, false, tempMenuSelections);

    Graphics_setFont(&g_sContext, &g_sFontCmss12);

    char* currentHoveredSelection_toString;

    while(1) {
        drawSelectableMenuSelections(&selectableMainMenuSelections);

        while (!moveUp && !moveDown && !joystickButtonPressed) {};

        selectableMainMenuSelections.selectionHasChangedOnce = true;
        currentHoveredSelection_toString = selectableMainMenuSelections.menuSelections[selectableMainMenuSelections.currentHoveredSelection];
        
        if (moveUp || moveDown) {
            changeSelectedMenuSelection(&selectableMainMenuSelections, moveUp, moveDown);
        }
        else if (joystickButtonPressed) {
            changePage(currentHoveredSelection_toString);
        }
        restartAndWaitTimerExpiration(200);
    };
}

int drawLavaLampTitleAndReturnOffsetY()
{
    
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

    int offsetY = 0;

    int i;
    for (i = 0; i < 8; i++) {
        Graphics_setForegroundColor(&g_sContext, lavaLampGradient[i]);
        if (i==7) {
            Graphics_drawString(&g_sContext, string_lavalamp, 1, 90, offsetY, true);
        }
        else Graphics_drawString(&g_sContext, string_lavalamp, 1, i*12, offsetY, true);
        chopN(string_lavalamp, 1);
        if (loadAnimation_user) restartAndWaitTimerExpiration(50);
    }

    offsetY += 20;

    int j;
    for (j = 0; j < 11; j++) {
        Graphics_setForegroundColor(&g_sContext, encryptGradient[j]);
        Graphics_drawString(&g_sContext, string_encrypt, 1, j*11, offsetY, true);
        chopN(string_encrypt, 1);
        if (loadAnimation_user) restartAndWaitTimerExpiration(50);
    }

    Graphics_setForegroundColor(&g_sContext, FOREGROUND_COLOR);

    Graphics_drawLineH(&g_sContext, 0, 128, 45);

    return offsetY;
}

// TODO: fix this function
void setMainMenuSelections(struct selectableMenuSelections* selectablemainmenuselections,int offsetY)
{
    int tempOffsetsY[MAIN_MENU_SELECTIONS] = {0, 16, 32, 48};
    int tempOffsetsX[MAIN_MENU_SELECTIONS] = {2, 2, 2, 2};

    bool tempHoveredSelection[MAIN_MENU_SELECTIONS] = {true, false, false, false};

    char * tempMenuSelections[MAIN_MENU_SELECTIONS] = {"Generated Seed", "Generate Random Number", "About", "Settings"};

    setSelectableMenuSections(selectablemainmenuselections, MAIN_MENU_SELECTIONS, 0, offsetY, tempOffsetsY, tempOffsetsX, tempHoveredSelection, false, tempMenuSelections);

}
