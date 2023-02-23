#include "esp_timer.h"
#include "telegram.h"
#include "camera.h"
#include "encryption.h"
#include <random>

WiFiClientSecure client;

sha256_hasher_t hasher;
uint8_t hashedNumberDefault[32] = {164, 189, 205, 253, 192, 100, 250, 155, 255, 112, 152, 127, 127, 111, 114, 75, 34, 72, 234, 87, 90, 23, 222, 123, 234, 65, 162, 1, 2, 3, 10, 8};
uint8_t *hashedNumber = hashedNumberDefault;

void initWiFi(){
  WiFi.disconnect(); // Disconnect from WiFi network, if WiFi was not disconnected properly last time
  delay(1000);       // wait for WiFi to disconnect

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  if (SERIAL_DEBUG) {
    Serial.print("Connecting to Wifi");
  }
  while (WiFi.status() != WL_CONNECTED) {
    if (SERIAL_DEBUG) { Serial.print("."); }
    delay(3000);
  }
  if (SERIAL_DEBUG) { Serial.println("WiFi connnected"); }
}

void setup() {
  if (SERIAL_DEBUG) { Serial.begin(115200); }

  Serial.setDebugOutput(false); // To get low level debug if activated in platformio.ini

  Serial1.begin(115200, SERIAL_8N1, RXData, TXData);

  initWiFi();

  // Mount SPIFFS file system
  if (!SPIFFS.begin(true)) {
    if (SERIAL_DEBUG) { Serial.println("An Error has occurred while mounting SPIFFS"); }
    ESP.restart();
  }else{
    delay(500);
    if (SERIAL_DEBUG) { Serial.println("SPIFFS mounted successfully"); }
  }

  initialiseCamera();
  client.setInsecure();

  hasher = sha256_hasher_new();
}

void genSeed(message *update = NULL){
    camera_fb_t *fb = NULL;
    fb = esp_camera_fb_get();

    uint8_t *rgb;

    capturePhotoSaveSpiffs(fb);
    if (update != NULL) { sendCameraPhoto(update->chat_id, fb); }
    readRGBImage(fb, rgb);

    esp_camera_fb_return(fb);
}

void checkReceiveChannel(){
  if (Serial1.available() > 0) {
    String receivedData = Serial1.readString();
    if (receivedData == "GetPhoto") {
      genSeed();
    }
  }
}

void telegramRequest(){
  message update = getUpdate(); // check for new telegram messages

  if (SERIAL_DEBUG) {
    Serial.printf("chat id: %d\n", update.chat_id);
    Serial.println(update.text.substring(0, sizeof(DECRYPTION_COMMAND) / sizeof(const char) - 1));
    Serial.println(update.text.substring(0, sizeof(DECRYPTION_COMMAND) / sizeof(const char) - 1) == DECRYPTION_COMMAND);
  }

  // 788963490 is my telegram id to avoid that anyone can get photos
  if (update.chat_id != 0 && update.text == "/photo" && (update.user_id == 788963490 || update.user_id == 213298805 || update.user_id == 206312359)) {
    update.reply("Uploading...");
    genSeed(&update);
  } else if (update.chat_id != 0 && update.text.substring(0, 4) == NUMBER_COMMAND) {
    numberCommand(&update);
  } else if (update.chat_id != 0 && update.text == GEN_COMMAND) {
    genCommand(&update, hashedNumber);
  } else if (update.chat_id != 0 && update.text.substring(0, sizeof(ENCRYPTION_COMMAND) / sizeof(const char) - 1) == ENCRYPTION_COMMAND) {
    cryptMessage(&update);
  } else if (update.chat_id != 0 && update.text.substring(0, sizeof(DECRYPTION_COMMAND) / sizeof(const char) - 1) == DECRYPTION_COMMAND) {
    decryptMessage(&update);
  }
}

void loop() {
  // check if the wifi is connected
  if (WiFi.status() != WL_CONNECTED) {
    if (SERIAL_DEBUG) { Serial.println("WiFi suddenly disconnected!"); }
    initWiFi();
  }

  if (SERIAL_DEBUG) { Serial.println("calling update..."); }

  checkReceiveChannel(); // check if the msp432 has made a request

  telegramRequest(); // check if there are any new messages

  delay(200);
}