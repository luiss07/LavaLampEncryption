#include "sha/sha256.h"
#include "esp_timer.h"
#include "telegram.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/pk.h"
#include "mbedtls/error.h"
#include <iostream>
#include <iomanip>
#include <random>
#include <sstream>
uint8_t *parseRandomNumber(uint8_t *rgb);

sha256_hasher_t hasher;

WiFiClientSecure client;

void initialiseCamera()
{
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
  /*
  if (psramFound()) {
    config.frame_size = FRAMESIZE_VGA // (640 x 480);
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  */
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    if (SERIAL_DEBUG)
    {
      Serial.printf("Camera init failed with error 0x%x", err);
    }
    ESP.restart();
  }
  else
  {
    if (SERIAL_DEBUG)
    {
      Serial.println("Camera init OK");
    }
  }
}
void setup()
{
  Serial.begin(115200);
  // To get low level debug if activated in platformio.ini
  Serial.setDebugOutput(false);

  WiFi.disconnect(); // Disconnect from WiFi network, if WiFi was not disconnected properly last time
  delay(1000);       // wait for WiFi to disconnect

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  if (SERIAL_DEBUG)
  {
    Serial.print("Connecting to Wifi");
  }
  while (WiFi.status() != WL_CONNECTED)
  {
    if (SERIAL_DEBUG)
    {
      Serial.print(".");
    }
    delay(3000);
  }

  if (SERIAL_DEBUG)
  {
    Serial.println();
    Serial.println("WiFi connected!");
  }

  // Mount SPIFFS file system
  if (!SPIFFS.begin(true))
  {
    if (SERIAL_DEBUG)
    {
      Serial.println("An Error has occurred while mounting SPIFFS");
    }
    ESP.restart();
  }
  else
  {
    delay(500);
    if (SERIAL_DEBUG)
    {
      Serial.println("SPIFFS mounted successfully");
    }
  }
  initialiseCamera();
  client.setInsecure();

  hasher = sha256_hasher_new();
}

bool checkPhoto(fs::FS &fs)
{
  File f_pic = fs.open(FILE_PHOTO);
  if (!f_pic)
  {
    return 0;
  }
  unsigned int pic_sz = f_pic.size();
  return (pic_sz > 100);
}

void capturePhotoSaveSpiffs(camera_fb_t *fb)
{
  // pointer
  bool ok = 0;      // Boolean indicating if the picture has been taken correctly
  int n_trying = 0; // Int to indicate how many times we tried to catch a picture
  do
  {
    // Take a photo with the camera
    if (SERIAL_DEBUG)
    {
      Serial.printf("Taking a photo... [ %d ]\n", n_trying);
    }

    if (!fb)
    {
      if (SERIAL_DEBUG)
      {
        Serial.println("Camera capture failed");
      }
      return;
    }

    // Photo file name
    if (SERIAL_DEBUG)
    {
      Serial.printf("Picture file name: %s\n", FILE_PHOTO);
    }

    File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);

    // Insert the data in the photo file
    if (!file)
    {
      if (SERIAL_DEBUG)
      {
        Serial.println("Failed to open file in writing mode");
      }
    }
    else
    {
      file.write(fb->buf, fb->len); // payload (image), payload length
      if (SERIAL_DEBUG)
      {
        Serial.print("The picture has been saved in ");
        Serial.print(FILE_PHOTO);
        Serial.print(" - Size: ");
        Serial.print(file.size());
        Serial.println(" bytes");
      }
    }
    // Close the file
    file.close();
    // esp_camera_fb_return(fb);

    delay(2000); // delay the saving check otherwise the check will happen too fast

    // check if file has been correctly saved in SPIFFS
    ok = checkPhoto(SPIFFS);

    if (!ok)
    {
      ++n_trying;
      if (SERIAL_DEBUG)
      {
        Serial.println("There was an error opening the photo!");
        Serial.printf("Retrying... [ %d ]\n", n_trying);
      }
    }

  } while (!ok && n_trying < 3);
}

bool readRGBImage(camera_fb_t *fb, uint8_t *rgb)
{
  uint32_t tTimer; // used to time tasks

  // check if psram is available
  if (!psramFound())
  {
    if (SERIAL_DEBUG)
    {
      Serial.println("ERR: no psram available to store the RGB data");
    }
    return false;
  }

  //   ------ main code for converting an image to RGB data ------

  tTimer = millis(); // get current running time                                                                      // store time that image capture started
  if (!fb)
  {
    if (SERIAL_DEBUG)
    {
      Serial.println("ERR: failed to capture image from camera");
    }
    return false;
  }
  else
  {
    if (SERIAL_DEBUG)
    {
      Serial.print("Image capture took ");
      Serial.print(millis() - tTimer);
      Serial.println(" ms");
      Serial.printf("Image resolution: %d x %d\n", fb->width, fb->height);
      Serial.printf("Image size: %d bytes\n", fb->len);
      Serial.printf("Image format: %d\n", fb->format);
      // Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    }
  }

  /*
  // display captured image using base64 - seems a bit unreliable especially with larger images?
  if (!sendRGBfile) {
    client.print("<br>Displaying image direct from frame buffer");
    String base64data = base64::encode(fb->buf, fb->len);      // convert buffer to base64
    client.print(" - Base64 data length = " + String(base64data.length()) + " bytes\n" );
    client.print("<br><img src='data:image/jpg;base64," + base64data + "'></img><br>\n");
  }
  */

  // allocate memory to store the rgb data (in psram, 3 bytes per pixel)
  if (SERIAL_DEBUG)
  {
    Serial.printf("Free psram before rgb data allocated = %d KB \n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);
  }
  void *ptrVal = NULL;                                // create a pointer for memory location to store the data
  uint32_t ARRAY_LENGTH = fb->width * fb->height * 3; // calculate memory required to store the RGB data (i.e. number of pixels in the jpg image x 3)
  if (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) < ARRAY_LENGTH)
  { // check if there is enough psram available
    if (SERIAL_DEBUG)
    {
      Serial.println("ERR: not enough psram to store the RGB data");
    }
    return false;
  }

  ptrVal = heap_caps_malloc(ARRAY_LENGTH, MALLOC_CAP_SPIRAM); // allocate memory for the RGB image
  // ptrVal = malloc(ARRAY_LENGTH);
  if (ptrVal == NULL)
  {
    Serial.println("ptrVal NULL");
    return false;
  }
  rgb = (uint8_t *)ptrVal; // create the 'rgb' array pointer to the allocated memory space
  bool jpeg_converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, rgb);
  if (SERIAL_DEBUG)
  {
    Serial.printf("Free psram after rgb data allocated = %d K\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);
  }

  // convert the captured jpg image (from frame buffer) to rgb data (store in 'rgb' array)
  tTimer = millis(); // store time that image conversion process started
  if (!jpeg_converted)
  {
    if (SERIAL_DEBUG)
    {
      Serial.println("ERR: failed to convert jpeg to rgb888");
    }
    return false;
  }
  if (SERIAL_DEBUG)
  {
    Serial.printf("Image conversion took %d ms\n", millis() - tTimer);
    // for (uint32_t i = 0; i < PTRVAL_LEN; i++)
    //{
    //   Serial.print(rgb[i]);
    // }
    uint8_t *randomNumber = parseRandomNumber(rgb);
    for (int i = 0; i < 32; i++)
    {
      Serial.printf("%d", randomNumber[i]);
    }
    Serial.println();
  }

  /* --- THINGS TO DO ---
    - eventually send rgb data through uart to msp432 OR elaborate the data on the esp32 and send it to the msp432
  */

  /*
    //   ****** examples of using the resulting RGB data *****

    // display some of the resulting data
        uint32_t resultsToShow = 50;                                                                       // how much data to display
        sendText(client,"<br>R,G,B data for first " + String(resultsToShow / 3) + " pixels of image");
        for (uint32_t i = 0; i < resultsToShow-2; i+=3) {
          sendText(client,String(rgb[i+2]) + "," + String(rgb[i+1]) + "," + String(rgb[i+0]));           // Red , Green , Blue
          // // calculate the x and y coordinate of the current pixel
          //   uint16_t x = (i / 3) % fb->width;
          //   uint16_t y = floor( (i / 3) / fb->width);
        }

    // find the average values for each colour over entire image
        uint32_t aRed = 0;
        uint32_t aGreen = 0;
        uint32_t aBlue = 0;
        for (uint32_t i = 0; i < (ARRAY_LENGTH - 2); i+=3) { // go through all data and add up totals
          aBlue+=rgb[i];
          aGreen+=rgb[i+1];
          aRed+=rgb[i+2];
        }
        aRed = aRed / (fb->width * fb->height); // divide total by number of pixels to give the average value
        aGreen = aGreen / (fb->width * fb->height);
        aBlue = aBlue / (fb->width * fb->height);
        sendText(client,"Average Blue = " + String(aBlue));
        sendText(client,"Average Green = " + String(aGreen));
        sendText(client,"Average Red = " + String(aRed));
  delay(5000);
  // finished with the data so free up the memory space used in psram
  //esp_camera_fb_return(fb); // camera frame buffer
  Serial.println("prima heap free");

  //free(ptrVal);
  */

  // only free ptrVal when we have stopped using it
  // heap_caps_free(ptrVal);

  heap_caps_free(ptrVal);
  return true; // rgb data

} // readRGBImage

uint8_t *parseRandomNumber(uint8_t *rgb)
{
  Sha256.initHmac(rgb, PTRVAL_LEN);
  uint8_t *result = Sha256.resultHmac();

  // for (uint32_t i = 0; i < 5; i++)
  //{
  //   Serial.printf("%d ", rgb[i]);
  // }
  // Serial.println();

  return result;
}

void loop()
{
  // check if the wifi is connected
  if (WiFi.status() != WL_CONNECTED)
  {
    if (SERIAL_DEBUG)
    {
      Serial.println("WiFi suddenly disconnected!");
    }

    WiFi.disconnect(); // reset wifi
    delay(1000);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    // wait for the wifi to connect
    if (SERIAL_DEBUG)
    {
      Serial.print("Connecting to WiFi");
    }
    while (WiFi.status() != WL_CONNECTED)
    {
      if (SERIAL_DEBUG)
      {
        Serial.print(".");
      }
      delay(3000);
    }
    if (SERIAL_DEBUG)
    {
      Serial.println();
      Serial.println("WiFi connnected");
    }
  }
  if (SERIAL_DEBUG)
  {
    Serial.println("calling update...");
  }
  message update = getUpdate();
  if (SERIAL_DEBUG)
  {
    Serial.printf("chat id: %d\n", update.chat_id);
  }
  // 788963490 is my telegram id to avoid that anyone can get photos
  if (update.chat_id != 0 && update.text == "/photo" &&
      (update.user_id == 788963490 || update.user_id == 213298805 || update.user_id == 206312359))
  {
    update.reply("Uploading...");
    camera_fb_t *fb = NULL;
    fb = esp_camera_fb_get();

    uint8_t *rgb;

    capturePhotoSaveSpiffs(fb);
    sendCameraPhoto(update.chat_id, fb);
    // delay(5000);
    readRGBImage(fb, rgb);

    esp_camera_fb_return(fb);
  }
  else if (update.chat_id != 0 && update.text.substring(0, 4) == NUMBER_COMMAND)
  {
    long int min = 0;
    long int max = 100;
    bool max_is_zero = false;
    char *pEnd;
    bool respond = true;
    if (update.text.length() > 4)
    {
      long int min_tmp, max_tmp;
      // to move the pointer to the beginning of the numbers
      // TODO handle case in which user puts numbers that are out of bound
      min_tmp = strtol(update.text.c_str() + sizeof(NUMBER_COMMAND) / sizeof(const char), &pEnd, 10);
      Serial.println(min_tmp);
      if (*(pEnd + 1) == '0' && *(pEnd + 2) == '\0')
      {
        max_is_zero = true;
      }
      max_tmp = strtol(pEnd, NULL, 10);
      Serial.print("Max ");
      Serial.println(max_tmp);
      if (min_tmp == 0)
      {
        update.reply("Invalid syntax: correct syntax is /num min max");
        respond = false;
      }
      else if (max_tmp == 0 && !max_is_zero)
      {
        max = min_tmp;
      }
      else
      {
        min = min_tmp;
        max = max_tmp;
      }
    }
    if (respond && min > max)
    {
      update.reply("Min value can't be bigger than max value");
      respond = false;
    }
    if (respond)
    {
      update.reply("1");
    }
  }

  else if (update.chat_id != 0 && update.text == GEN_COMMAND)
  {
    uint8_t hashedNumber[32] = {164, 189, 205, 253, 192, 177, 250, 155, 255, 112, 152, 127, 127, 111, 114, 75, 34, 72, 234, 87, 90, 23, 222, 123, 234, 65, 162, 1, 2, 3, 10, 8};
    mbedtls_aes_context aes;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    String seed_str = String(hashedNumber[0]);
    int last_i = 0;
    for (int i = 1; i < 32 && (seed_str + String(hashedNumber[i])).length() < 33; i++)
    {
      seed_str += String(hashedNumber[i]);
      last_i = i;
    }
    if (seed_str.length() < 32)
    {
      for (int i = 0; seed_str.length() != 32 && i < String(hashedNumber[last_i]).length(); i++)
      {
        seed_str[seed_str.length()] = String(hashedNumber[last_i])[i];
        last_i++;
      }
    }
    unsigned char iv[16];

    // seed the entropy source manually based on a user-provided seed
    mbedtls_entropy_update_manual(&entropy, (const unsigned char *)&hashedNumber, sizeof(hashedNumber));

    // initialize the random number generator
    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);

    // generate a random 128-bit IV
    mbedtls_ctr_drbg_random(&ctr_drbg, iv, 16);
    update.reply(String("Key generated: ") + seed_str);
    update.reply(String("IV generated: ") + String(iv, 16));
    mbedtls_entropy_free(&entropy);
    mbedtls_aes_free(&aes);
    mbedtls_ctr_drbg_free(&ctr_drbg);

  }
  else if (update.chat_id != 0 && update.text == "decrypt")
  {
    mbedtls_aes_context ctx;
    
    // TODO fix small things about this
    mbedtls_aes_init(&ctx);
    // this should be only the key that should be 32 bits
    mbedtls_aes_setkey_dec(&ctx, (const unsigned char *)update.text.c_str(), 256); // set decryption key

    // decrypt data
    // Will be taken from user input or in some other way
    int data_size = 32;
    unsigned char ciphertext[32];
    unsigned char decrypted[32];
    // in this case update.text.c_str() is the iv that will be taken from user
    mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_DECRYPT, data_size, (unsigned char *)update.text.c_str(), ciphertext, decrypted);

    // remove padding (if any)
    size_t padding = decrypted[data_size - 1];
    size_t padded_size = data_size + padding;
    if (padding > 0 && padding <= 16)
    {
      for (size_t i = data_size - padding; i < data_size; i++)
      {
        if (decrypted[i] != padding)
        {
          // invalid padding
          break;
        }
      }
      padded_size = data_size - padding;
    }

    // print decrypted plaintext
    update.reply(String((const char *) &decrypted));

    mbedtls_aes_free(&ctx);
  }
  delay(200);
}