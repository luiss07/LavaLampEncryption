#include "telegram.h"
int offset = 0;

message send_request(String method, String api_request, String params)
{
  message update;

  if (method == "POST" && api_request == "sendPhoto")
  {
    // The request string is the call to the telegram api
    String request = method + " /bot" + TOKEN + "/" + api_request + " HTTP/1.1";
    String host_line = "Host: " + String(url);
    Serial.println("Uploading...");
    client.connect(url, HTTP_PORT);
    if (!client.connected())
    {
      Serial.println("Client could not connect!");
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
    String request = method + " /bot" + TOKEN + "/" + api_request + "?" + params + " HTTP/1.1\r\nHost: " + url + "\r\n";
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
        Serial.printf("update.chat_id: %d, update.offset_id %d\n", update.chat_id, update.offset_id);
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
    else {
      Serial.printf("Client couldn't connect to URL %s and PORT %d\n", url, HTTP_PORT);
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

message sendMessage(int64_t chat_id, String text, String parse_mode)
{
  return send_request("POST", "sendMessage", "chat_id=" + String(chat_id) + "&text=" + text + "&parse_mode=" + parse_mode);
}

message sendCameraPhoto(int64_t chat_id, camera_fb_t *fb)
{
  String params = "--Lava01\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + String(chat_id) +
                  "\r\n--Lava01\r\nContent-Disposition: form-data; name=\"photo\"" + "; filename=\"lavalamp.jpeg\"\r\nContent-Type: image/jpeg \r\n";
  return send_request("POST", "sendPhoto", params);
}

message message::reply(String text, String parse_mode)
{
  return send_request("POST", "sendMessage", "chat_id=" + String(this->chat_id) + "&text=" + text + "&parse_mode=" + parse_mode + "&reply_to_message" + String(this->message_id));
}
