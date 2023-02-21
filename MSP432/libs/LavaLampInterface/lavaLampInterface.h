#ifndef LAVALAMPINTERFACE_H
#define LAVALAMPINTERFACE_H

#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include "LcdDriver/HAL_MSP_EXP432P401R_Crystalfontz128x128_ST7735.h"
#include <stdio.h>
#include <math.h>

#include "../shared.h"

#define ROWS_FOR_HASH_NUMBER 5
#define DIGITS_OF_HASHED_NUMBER 97
#define DIGITS_OF_HASHED_NUMBER_ARRAY 4
#define MAX_DIGITS_INLINE 19
//#define MAX_SELECTABLE_SELECTIONS 5
#define MAIN_MENU_SELECTIONS 4
//#define GENERATE_PAGE_SELECTIONS 5
#define GENERATE_PAGE_SELECTIONS 4
#define SETTINGS_PAGE_SELECTIONS 3
#define MAX_RANDOM_NUMBER_DIGITS 10
#define MAX_THEMES 3

struct selectableMenuSelections
{
    int numberOfSelections;
    int currentHoveredSelection;
    int offsetY;
    int *offsetsY;
    int *offsetsX;

    // array to remember which selection we are hovering on
    bool *hoveredSelection;
    bool selectionHasChangedOnce;

    // array of the different menu selections
    char **menuSelections;

    char *min_to_string;
    char *max_to_string;
    long long int min;
    long long int max;
};

void mainMenuPage();

#endif
