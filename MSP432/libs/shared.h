#ifndef SHARED_H
#define SHARED_H

extern int joystickButtonPressed;
extern int rightUpButtonPressed;
extern int rightDownButtonPressed;

extern uint16_t moveUp;
extern uint16_t moveDown;
extern uint16_t moveLeft;
extern uint16_t moveRight;

extern uint8_t hashedNumber[32];

/* Graphic library context */
extern Graphics_Context g_sContext;

extern uint32_t FOREGROUND_COLOR;
extern uint32_t BACKGROUND_COLOR;
extern uint32_t SELECTION_COLOR;

#endif