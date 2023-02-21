#ifndef STRUCT_MESSAGE

#define STRUCT_MESSAGE

#include <esp_camera.h>
#include <SPIFFS.h>
#include <WiFiClientSecure.h>
#include "ArduinoJson.h"
#include "config.h"
#include "secrets.h"


inline WiFiClientSecure client;

struct message
{
    int64_t chat_id = 0;
    int64_t user_id = 0;
    int64_t message_id = 0;
    int64_t offset_id = 0;
    String text;
    bool is_private = false;
    message reply(String text, String parse_mode = "None");
};
message send_request(String method, String api_request, String params);
message getUpdate();
message sendMessage(int64_t chat_id, String text, String parse_mode = "None");
message sendCameraPhoto(int64_t chat_id, camera_fb_t *fb);
#endif
