#ifndef web_request
#define web_request

#include <Arduino.h>

struct request_payload
{
    String value;
    int http_code;
};

request_payload http_get_request(String);
request_payload https_get_request(String, const char *);

#endif