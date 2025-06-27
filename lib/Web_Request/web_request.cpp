#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>

#include "web_request.h"

request_payload _generic_request(WiFiClient client, String request)
{
    HTTPClient http;    
    request_payload answer;

    // Your IP address with path or Domain name with URL path 
    if (http.begin(client, request))
    {
        answer.http_code = http.GET();

        if (answer.http_code > 0) 
        {
            if (answer.http_code == HTTP_CODE_OK || answer.http_code == HTTP_CODE_MOVED_PERMANENTLY)
            {
                answer.value = http.getString();
            }
        }
        else 
        {
            answer.value = http.errorToString(answer.http_code).c_str();
        }
    // Free resources
    http.end();
    }
    else
    {
        answer.value = "[HTTP] Unable to connect";
        answer.http_code = 0;
    }
    return answer;
}

request_payload _generic_request(WiFiClientSecure client, String request)
{
    HTTPClient http;    
    request_payload answer;

    // Your IP address with path or Domain name with URL path 
    if (http.begin(client, request))
    {
        answer.http_code = http.GET();

        if (answer.http_code > 0) 
        {
            if (answer.http_code == HTTP_CODE_OK || answer.http_code == HTTP_CODE_MOVED_PERMANENTLY)
            {
                answer.value = http.getString();
            }
        }
        else 
        {
            answer.value = http.errorToString(answer.http_code).c_str();
        }
    // Free resources
    http.end();
    }
    else
    {
        answer.value = "[HTTP] Unable to connect";
        answer.http_code = 0;
    }
    return answer;
}

request_payload http_get_request(String request) 
{
    WiFiClient client;
    HTTPClient http;
    return _generic_request(client, request);
}

request_payload https_get_request(String request, const char * certificate) 
{
    WiFiClientSecure client;
    HTTPClient https;  
    X509List cert(certificate);
    client.setTrustAnchors(&cert);
    return _generic_request(client, request);
}