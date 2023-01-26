#include <WiFiClientSecure.h>
#include "esp_camera.h"

#define FILE_PHOTO "/photo.jpeg"

const char *url = "api.telegram.org";
int HTTP_PORT = 443;
int offset = 0;
WiFiClientSecure client;
struct message
{
  int64_t chat_id = 0;
  int64_t user_id = 0;
  int64_t message_id = 0;
  int64_t offset_id = 0;
  String text;
  bool is_private = false;
  message reply(String text, String parse_mode = "None"){
    return send_request("POST", "sendMessage", "chat_id=" + String(this->chat_id) + "&text=" + text + "&parse_mode=" + parse_mode +"&reply_to_message" + String(this->message_id));
  }
};
message send_request(String method, String api_request, String params);
message getUpdate();
message sendMessage(int64_t chat_id, String text, String parse_mode = "None");
message sendCameraPhoto(int64_t chat_id, camera_fb_t *fb);
