#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <esp_camera.h>
#include <SPIFFS.h>

void initialiseCamera();
bool checkPhoto(fs::FS &fs);
void capturePhotoSaveSpiffs(camera_fb_t *fb);

#endif