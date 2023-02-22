#ifndef __ENCRYPTION_H__
#define __ENCRYPTION_H__

#include "config.h"
#include "telegram.h"
#include "sha/sha256.h"
#include <esp_camera.h>
#include <SPIFFS.h>
#include <sstream>

extern sha256_hasher_t hasher;
extern uint8_t *hashedNumber;

uint8_t *parseRandomNumber(uint8_t *rgb);
bool readRGBImage(camera_fb_t *fb, uint8_t *rgb);
void cryptMessage(message *update);
void decryptMessage(message *update);

#endif