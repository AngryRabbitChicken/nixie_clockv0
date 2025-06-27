#ifndef web_request
#define web_request

#include <Arduino.h>

String http_get_request(String);
String https_get_request(String, const char *);

#endif