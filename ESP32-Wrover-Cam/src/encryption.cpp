#include "encryption.h"


uint8_t *parseRandomNumber(uint8_t *rgb) {
  Sha256.initHmac(rgb, PTRVAL_LEN);
  uint8_t *result = Sha256.resultHmac();

  return result;
}

bool readRGBImage(camera_fb_t *fb, uint8_t *rgb) {
  uint32_t tTimer; // used to time tasks

  // check if psram is available
  if (!psramFound()) {
    if (SERIAL_DEBUG) { Serial.println("ERR: no psram available to store the RGB data"); }
    return false;
  }

  //   ------ main code for converting an image to RGB data ------

  tTimer = millis(); // get current running time                                                                      // store time that image capture started
  if (!fb) {
    if (SERIAL_DEBUG) {
      Serial.println("ERR: failed to capture image from camera");
    }
    return false;
  } else {
    if (SERIAL_DEBUG) {
      Serial.print("Image capture took ");
      Serial.print(millis() - tTimer);
      Serial.println(" ms");
      Serial.printf("Image resolution: %d x %d\n", fb->width, fb->height);
      Serial.printf("Image size: %d bytes\n", fb->len);
      Serial.printf("Image format: %d\n", fb->format);
    }
  }

  // allocate memory to store the rgb data (in psram, 3 bytes per pixel)
  if (SERIAL_DEBUG) { Serial.printf("Free psram before rgb data allocated = %d KB \n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024); }

  void *ptrVal = NULL;                                // create a pointer for memory location to store the data
  uint32_t ARRAY_LENGTH = fb->width * fb->height * 3; // calculate memory required to store the RGB data (i.e. number of pixels in the jpg image x 3)
  if (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) < ARRAY_LENGTH) { // check if there is enough psram available
    if (SERIAL_DEBUG) { Serial.println("ERR: not enough psram to store the RGB data"); }
    return false;
  }

  ptrVal = heap_caps_malloc(ARRAY_LENGTH, MALLOC_CAP_SPIRAM); // allocate memory for the RGB image
  // ptrVal = malloc(ARRAY_LENGTH);
  if (ptrVal == NULL) {
    if (SERIAL_DEBUG) { Serial.println("ptrVal NULL"); }
    return false;
  }

  rgb = (uint8_t *)ptrVal; // create the 'rgb' array pointer to the allocated memory space
  bool jpeg_converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, rgb);
  if (SERIAL_DEBUG) { Serial.printf("Free psram after rgb data allocated = %d K\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024); }

  // convert the captured jpg image (from frame buffer) to rgb data (store in 'rgb' array)
  tTimer = millis(); // store time that image conversion process started
  if (!jpeg_converted) {
    if (SERIAL_DEBUG) { Serial.println("ERR: failed to convert jpeg to rgb888"); }
    return false;
  }

  hashedNumber = parseRandomNumber(rgb);
  String seed_str = String(hashedNumber[0]);
  int last_i = 0;
  for (int i = 1; i < 32 && (seed_str + String(hashedNumber[i])).length() < 33; i++) {
    seed_str += String(hashedNumber[i]);
    last_i = i;
  }

  if (seed_str.length() < 32) {
    for (int i = 0; seed_str.length() != 32 && atoll((seed_str + String(hashedNumber[i])).c_str()); i++)
    {
      seed_str += String(hashedNumber[last_i])[i];
      last_i++;
    }
    srand(atoll(seed_str.c_str()));
  }

  Serial1.write(hashedNumber, 32); // send the new seed to the ESP32

  if (SERIAL_DEBUG) {
    Serial.printf("Image conversion took %d ms\n", millis() - tTimer);
    for (int i = 0; i < 32; i++) {
      Serial.printf("%d", hashedNumber[i]);
    }
    Serial.println();
  }

  heap_caps_free(ptrVal);
  return true; // rgb data

} // readRGBImage

void cryptMessage(message *update){
    unsigned char plaintext[CIPHERTEXT_SIZE];
    unsigned char ciphertext[CIPHERTEXT_SIZE];

    const char *actual_pos = update->text.c_str();
    actual_pos += sizeof(ENCRYPTION_COMMAND) / sizeof(const char);
    if (update->text.length() < (sizeof(ENCRYPTION_COMMAND) / sizeof(const char) + 32) || *(actual_pos - 1) != ' ') {
      update->reply(String("Invalid syntax, correct syntax: /crypt KEY(long 32 char) message"));
      return;
    }
    unsigned char key[32];
    for (int i = 0; i < 32; i++, actual_pos++) {
      key[i] = *actual_pos;
    }

    actual_pos++;
    int plain_len = 0;
    for (int i = 0; i < PLAINTEXT_SIZE && actual_pos != update->text.end(); i++, actual_pos++, plain_len++) {
      plaintext[i] = *actual_pos;
    }
    Serial.println(plain_len);
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    if (plain_len % 16 != 0) {
      int padding_len = 16 - plain_len % 16;
      for (int i = plain_len; i < padding_len; i++) {
        plaintext[i] = (char)padding_len;
      }
      plain_len += padding_len;
    }
    plaintext[plain_len] = '\0';

    // set the AES key and IV
    int cipher_len = ((plain_len) / 16);
    if ((plain_len) % 16 != 0) {
      cipher_len += 1;
    }
    if (SERIAL_DEBUG) { Serial.println(plain_len); }
    cipher_len *= 32;
    mbedtls_aes_setkey_enc(&aes, key, 256);
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, plaintext, ciphertext);
    mbedtls_aes_free(&aes);
    std::stringstream ss;
    ss << "Encrypted message: ";

    // Set the output stream to print in hexadecimal format
    ss << std::hex;

    // Iterate through the ciphertext array and add each byte to the stringstream
    for (size_t i = 0; i < cipher_len; i++) {
      ss << static_cast<unsigned>(ciphertext[i]);
    }
    ss << '\0';
    update->reply(String(ss.str().c_str()));
}

void decryptMessage(message *update){
    mbedtls_aes_context aes;

    mbedtls_aes_init(&aes);
    unsigned char decrypted[CIPHERTEXT_SIZE];
    unsigned char ciphertext[CIPHERTEXT_SIZE];
    const char *actual_pos = update->text.c_str();
    actual_pos += sizeof(DECRYPTION_COMMAND) / sizeof(const char);
    if (update->text.length() < (sizeof(DECRYPTION_COMMAND) / sizeof(const char) + 32) || *(actual_pos - 1) != ' ') {
      update->reply(String("Invalid syntax, correct syntax: /decrypt KEY(long 32 char) message"));
      return;
    }
    unsigned char key[32];
    for (int i = 0; i < 32; i++, actual_pos++) {
      key[i] = *actual_pos;
    }
    mbedtls_aes_setkey_dec(&aes, key, 256); // set decryption key
    actual_pos++;
    int plain_len = 0;
    for (int i = 0; i < CIPHERTEXT_SIZE && actual_pos != update->text.end(); i++, actual_pos++, plain_len++) {
      ciphertext[i] = *actual_pos;
    }
    plain_len /= 2;
    // decrypt data
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, ciphertext, decrypted);

    mbedtls_aes_free(&aes);
    std::stringstream ss;
    ss << "Decrypted message: ";

    // Set the output stream to print in hexadecimal format
    ss << std::hex;

    // Iterate through the ciphertext array and add each byte to the stringstream
    for (size_t i = 0; i < plain_len; i++) {
      ss << static_cast<unsigned>(decrypted[i]);
    }
    ss << '\0';
    update->reply(String(ss.str().c_str()));
}