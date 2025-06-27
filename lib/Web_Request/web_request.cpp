#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>

#include "web_request.h"

String https_get_request(String request, const char *certificate)
{
  WiFiClientSecure client;
  HTTPClient https;
  String payload;
  X509List cert(certificate);
  client.setTrustAnchors(&cert);
  // client.setInsecure();

  if (https.begin(client, request))
  { // HTTPS
    // start connection and send HTTP header
    int httpCode = https.GET();

    // httpCode will be negative on error
    if (httpCode > 0)
    {
      // HTTP header has been sent and Server response header has been handled
      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
      {
        payload = https.getString();
      }
    }
    else
    {
      payload = https.errorToString(httpCode).c_str();
    }

    https.end();
  }
  else
  {
    payload = "[HTTPS] Unable to connect";
  }
  return payload;
}

String http_get_request(String serverName) {
  WiFiClient client;
  HTTPClient http;
    
  // Your IP address with path or Domain name with URL path 
  http.begin(client, serverName);
  
  // If you need Node-RED/server authentication, insert user and password below
  //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}