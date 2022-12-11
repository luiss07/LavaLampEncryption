#include <Wifi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "esp_camera.h"
#include "esp_timer.h"
#include "secrets.h"
#include <SPIFFS.h>
#define SERIAL_DEBUG 1 // enable comments print in serial monitor
#define JSON_DESER_VERBOSE 0

#define FILE_PHOTO "/photo.jpeg"
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 21
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 19
#define Y4_GPIO_NUM 18
#define Y3_GPIO_NUM 5
#define Y2_GPIO_NUM 4
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

struct message
{
  int64_t chat_id = 0;
  int64_t user_id = 0;
  int64_t message_id = 0;
  int64_t offset_id = 0;
  String text;
  bool is_private = false;
};
const char *url = "api.telegram.org";
int HTTP_PORT = 443;
int offset = 0;

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
  Serial.setDebugOutput(true);
  WiFi.begin(ssid, password);
  Serial.println("Starting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
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

message send_request(String method, String api_request, String params, camera_fb_t *fb = NULL)
{
  message update;
  
  if (method == "POST" && api_request == "sendPhoto")
  {
    // The request string is the call to the telegram api
    String request = method + " /bot" + token + "/" + api_request + " HTTP/1.1";
    String host_line = "Host: " + String(url);
    Serial.println("Uploading...");
    client.connect(url, HTTP_PORT);
    if (!client.connected())
    {
      client.stop();
      return update;
    }
    // the params are the sections of the multipart formdata
    String start_request = params;
    String end_request = "\r\n--Lava01--\r\n";
    File photo = SPIFFS.open(FILE_PHOTO, "r");
    uint16_t imageLen = photo.size();
    int total_len = start_request.length() + end_request.length() + imageLen;
    // Here the client starts to write the request
    client.println(request);
    client.println(host_line);
    // The boundary is also in the params to devide them
    client.println(F("Content-Type: multipart/form-data; boundary=Lava01"));
    client.print(F("Content-Length: "));
    client.println(String(total_len));
    client.println();
    client.println(start_request);
    // Loading the photo 256bytes at a time (it can become lower if needed)
    byte buffer[256];
    int count = 0;
    while (photo.available())
    {
      buffer[count] = photo.read();
      count++;
      if (count == 256)
      {
        client.write((const uint8_t *)buffer, 256);
        count = 0;
      }
    }
    if (count > 0)
    {
      client.write((const uint8_t *)buffer, count);
    }
    client.print(end_request);
    // At the moment there is no use for the response;
    Serial.println(client.readString());
    photo.close();
    client.stop();
    Serial.println("done...");
  }
  else if (method == "GET" || method == "POST")
  {
    String request = method + " /bot" + token + "/" + api_request + "?" + params + " HTTP/1.1\r\nHost: " + url + "\r\n";
    request += "Connection: close\r\n\r\n";
    if (client.connect(url, HTTP_PORT))
    {
      // if connected:
      client.println(request);
      if (client.println() == 0)
      {
        Serial.println(F("Failed to send request"));
        client.stop();
        return update;
      }

      // Check HTTP status
      char status[32] = {0};
      client.readBytesUntil('\r', status, sizeof(status));
      if (strcmp(status, "HTTP/1.1 200 OK") != 0)
      {
        if (JSON_DESER_VERBOSE)
        {
          Serial.print(F("Unexpected response: "));
          Serial.println(status);
        }
        client.stop();
        return update;
      }

      // Skip HTTP headers
      char endOfHeaders[] = "\r\n\r\n";
      if (!client.find(endOfHeaders))
      {
        Serial.println(F("Invalid response"));
        client.stop();
        return update;
      }

      // Parse JSON object
      StaticJsonDocument<2000> doc;
      DeserializationError error = deserializeJson(doc, client);
      if (error)
      {
        if (error == DeserializationError::NoMemory)
        {
          offset++;
        }
        if (JSON_DESER_VERBOSE)
        {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
        }
        client.stop();
        return update;
      }

      // deserialization was succesful
      if (api_request == "getUpdates")
      {
        update.is_private = doc["result"][0]["message"]["chat"]["type"] == "private";
        update.chat_id = doc["result"][0]["message"]["chat"]["id"].as<int64_t>();
        update.user_id = doc["result"][0]["message"]["from"]["id"].as<int64_t>();
        update.message_id = doc["result"][0]["message"]["message_id"].as<int64_t>();
        update.text = doc["result"][0]["message"]["text"].as<String>();
        offset = (int)doc["result"][0]["update_id"];
        update.offset_id = offset;
        offset++;
      }
      else
      {
        update.chat_id = doc["result"]["chat"]["id"].as<int64_t>();
        update.is_private = doc["result"]["chat"]["type"].as<String>() == "private";
        update.user_id = doc["result"]["from"]["id"].as<int64_t>();
        update.message_id = doc["result"]["message_id"].as<int64_t>();
        update.text = doc["result"]["text"].as<String>();
      }
    }
  }

  else
  {

    Serial.println("Unrecognized method");
  }
  client.stop();
  return update;
}

message getUpdate()
{
  // Limit and offset are necessary to get one update every time
  return send_request("GET", "getUpdates", "limit=1&offset=" + String(offset));
}

message sendMessage(int64_t chat_id, String text, String parse_mode = "None")
{
  return send_request("POST", "sendMessage", "chat_id=" + String(chat_id) + "&text=" + text + "&parse_mode=" + parse_mode);
}

message sendCameraPhoto(int64_t chat_id, camera_fb_t *fb)
{
  String params = "--Lava01\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + String(chat_id) + 
  "\r\n--Lava01\r\nContent-Disposition: form-data; name=\"photo\"" + "; filename=\"lavalamp.jpeg\"\r\nContent-Type: image/jpeg \r\n";
  return send_request("POST", "sendPhoto", params);
}

bool readRGBImage(camera_fb_t *fb)
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
  if (!fb) {
    if (SERIAL_DEBUG) { Serial.println("ERR: failed to capture image from camera"); }
    return false;
  } else{
    if (SERIAL_DEBUG) {
      Serial.print("Image capture took ");
      Serial.print(millis() - tTimer);
      Serial.println(" ms");
      Serial.printf("Image resolution: %d x %d\n", fb->width, fb->height);
      Serial.printf("Image size: %d bytes\n", fb->len);
      Serial.printf("Image format: %d\n", fb->format);
      //Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
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
  if (SERIAL_DEBUG) { Serial.printf("Free psram before rgb data allocated = %d KB \n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM)/ 1024); }
  void* ptrVal = NULL;                                // create a pointer for memory location to store the data
  uint32_t ARRAY_LENGTH = fb->width * fb->height * 3; // calculate memory required to store the RGB data (i.e. number of pixels in the jpg image x 3)
  if (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) < ARRAY_LENGTH) { // check if there is enough psram available
    if (SERIAL_DEBUG) { Serial.println("ERR: not enough psram to store the RGB data"); }
    return false;
  }

  ptrVal = heap_caps_malloc(ARRAY_LENGTH, MALLOC_CAP_SPIRAM); // allocate memory for the RGB image
  //ptrVal = malloc(ARRAY_LENGTH);
  if (ptrVal == NULL){ Serial.println("ptrVal NULL"); return false; }
  uint8_t *rgb = (uint8_t *)ptrVal;                           // create the 'rgb' array pointer to the allocated memory space
  bool jpeg_converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, rgb);
  if (SERIAL_DEBUG) { Serial.printf("Free psram after rgb data allocated = %d K\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024); }

  // convert the captured jpg image (from frame buffer) to rgb data (store in 'rgb' array)
  tTimer = millis(); // store time that image conversion process started
  if (!jpeg_converted) {
    if (SERIAL_DEBUG) { Serial.println("ERR: failed to convert jpeg to rgb888");}
    return false;
  }
  if (SERIAL_DEBUG) { Serial.printf("Image conversion took %d ms", millis() - tTimer);}

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
  heap_caps_free(ptrVal); 
  Serial.println("prima heap free");
  return true;  // rgb data
  //heap_caps_free(ptrVal);

} // readRGBImage

void loop()
{
  message update = getUpdate();
  // 788963490 is my telegram id to avoid that anyone can get photos
  if (update.chat_id != 0 && update.text == "/photo" && update.user_id == 788963490)
  {
    camera_fb_t *fb = NULL;
    fb = esp_camera_fb_get();

    capturePhotoSaveSpiffs(fb);
    sendCameraPhoto(update.chat_id, fb);
    // delay(5000);
    // readRGBImage(fb);

    esp_camera_fb_return(fb);
  }

  delay(200);
}