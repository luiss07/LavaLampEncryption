#include "config.h"
#include "camera.h"

void initialiseCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // setting frame size, we use fixed resolution even if psram is available
  config.frame_size = FRAMESIZE_VGA; // (640 x 480);
  config.jpeg_quality = 16;
  config.fb_count = 1;

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    if (SERIAL_DEBUG) {
      Serial.printf("Camera init failed with error 0x%x", err);
    }
    ESP.restart();
  } else {
    if (SERIAL_DEBUG) {
      Serial.println("Camera init OK");
    }
  }
}

bool checkPhoto(fs::FS &fs) {
  File f_pic = fs.open(FILE_PHOTO);
  if (!f_pic) { return 0; }
  unsigned int pic_sz = f_pic.size();
  return (pic_sz > 100);
}

void capturePhotoSaveSpiffs(camera_fb_t *fb) {
  // pointer
  bool ok = 0;      // Boolean indicating if the picture has been taken correctly
  int n_trying = 0; // Int to indicate how many times we tried to catch a picture
  do {
    // Take a photo with the camera
    if (SERIAL_DEBUG) { Serial.printf("Taking a photo... [ %d ]\n", n_trying); }

    if (!fb) {
      if (SERIAL_DEBUG) { Serial.println("Camera capture failed"); }
      return;
    }

    // Photo file name
    if (SERIAL_DEBUG) { Serial.printf("Picture file name: %s\n", FILE_PHOTO); }

    File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);

    // Insert the data in the photo file
    if (!file) {
      if (SERIAL_DEBUG) { Serial.println("Failed to open file in writing mode"); }
    } else {
      file.write(fb->buf, fb->len); // payload (image), payload length
      if (SERIAL_DEBUG) {
        Serial.print("The picture has been saved in ");
        Serial.print(FILE_PHOTO);
        Serial.print(" - Size: ");
        Serial.print(file.size());
        Serial.println(" bytes");
      }
    }
    // Close the file
    file.close();

    delay(2000); // delay the saving check otherwise the check will happen too fast

    // check if file has been correctly saved in SPIFFS
    ok = checkPhoto(SPIFFS);

    if (!ok) {
      ++n_trying;
      if (SERIAL_DEBUG) {
        Serial.println("There was an error opening the photo!");
        Serial.printf("Retrying... [ %d ]\n", n_trying);
      }
    }

  } while (!ok && n_trying < 3);
}