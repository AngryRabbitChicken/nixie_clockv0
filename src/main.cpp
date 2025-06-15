#include <Arduino.h>
#include <SPI.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <time.h>
#include <ArduinoJson.h>

#include "Nixie_Controller.h"
#include "certificates.h"

#define CS D8 // The chip select pin connected to the shift regs is pin D8.

// put function declarations here:

// turn on and off the high voltage PSU for the nixies
void turn_hv_on();
void turn_hv_off();

// Check the buffer for integrity and send it.
void send_nixie_buffer();

// Print the NTP time
void printLocalTime();

// https requests
String https_request(String, const char *);

// general
uint32_t t_current, t_sys;

// Nixie control related
const uint8_t num_nixies = 6;
const uint8_t digits_per_nixie = 10;
volatile uint8_t digit_counter = 0; // Remove later, this just for testing
Nixie_Controller nxc;

// Time request related
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600 * 1;
const int daylightOffset_sec = 3600 * 0;

// HTTP request related

// In case we need help later, the webreq stuff is copied from here:
/*
  Complete project details: https://RandomNerdTutorials.com/esp8266-nodemcu-https-requests/
  Based on the example created by Ivan Grokhotkov, 2015 (File > Examples > ESP8266WiFi > HTTPSRequests)
*/
WiFiClientSecure client;
int HTTP_PORT = 443;
String HTTP_METHOD = "GET"; // or POST
char HOST_NAME[] = "timeapi.io";
String PATH_NAME = "/api/time/current/zone";
String queryString = "?timeZone=Europe%2FAmsterdam";
char TIMEAPI_CERT[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIEADCCAuigAwIBAgIQams/S+smTUKPx6y24Fj9MjANBgkqhkiG9w0BAQsFADCB\n"
    "gTE6MDgGA1UECwwxZ2VuZXJhdGVkIGJ5IEF2YXN0IEFudGl2aXJ1cyBmb3IgU1NM\n"
    "L1RMUyBzY2FubmluZzEeMBwGA1UECgwVQXZhc3QgV2ViL01haWwgU2hpZWxkMSMw\n"
    "IQYDVQQDDBpBdmFzdCBXZWIvTWFpbCBTaGllbGQgUm9vdDAeFw0xMDAxMDExMjAw\n"
    "MDBaFw00MDAxMDExMjAwMDBaMIGBMTowOAYDVQQLDDFnZW5lcmF0ZWQgYnkgQXZh\n"
    "c3QgQW50aXZpcnVzIGZvciBTU0wvVExTIHNjYW5uaW5nMR4wHAYDVQQKDBVBdmFz\n"
    "dCBXZWIvTWFpbCBTaGllbGQxIzAhBgNVBAMMGkF2YXN0IFdlYi9NYWlsIFNoaWVs\n"
    "ZCBSb290MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAq1irjCL+pdGL\n"
    "8Pn6L/AocZDWXSzsusO+BZZq3vIHzUUG4z4QNxouAAsdXaucqBt1nLkuFNMiOs6h\n"
    "Lf8KFsLnqfPG1VvRjJwPTSFHgVB9IcVkP8R9OBzqQPHCRtUIMoViiDdeR+ZH/wJW\n"
    "NHSAFFnJdHF11JpkxQyhO97FB15xvXAcN71sXpwekTkOjBIzsMrv51uRpL7K03p8\n"
    "9Mus4JdtO8Bga9Cz5vcp0hMlVnA9RqL74FdOr1kU6b0Wm7BBJ/gFxhhscV63Q/eU\n"
    "SL4Wh5lY1WJV+jWMLc92aFhDAQMyNV6p10xVLdHDtx52IKpRf10ANPKbnZDzfgPK\n"
    "bxXScvdqWQIDAQABo3IwcDAMBgNVHRMEBTADAQH/MAsGA1UdDwQEAwICBDATBgNV\n"
    "HSUEDDAKBggrBgEFBQcDATAdBgNVHQ4EFgQUGywRH4ftX7K4tlQIGD6ldKZPN6ow\n"
    "HwYDVR0jBBgwFoAUGywRH4ftX7K4tlQIGD6ldKZPN6owDQYJKoZIhvcNAQELBQAD\n"
    "ggEBAD51IleaRHDMbQd8Gx7088VwUiVYnvTaIN45j+iva/lv8jyYQ+4HbJkMIvxJ\n"
    "ZrsOwdSDKiSeAHiGPCnMW2ljfJoNJk3/ZyPxH7dNMakrHSYB86tO/T32MK8iyX7I\n"
    "S9UUFox8cc2FYRsS8D34fiXMI8jPU49PAG8GqrYQUMuvdaIoqL6rrgmzpVbMb+Ps\n"
    "z1dhK23qys1aZaf2TedD0Guxswp7zgu7NVpukMMQbzXGZLiBo86suAGUeBSD+dql\n"
    "BKppIU2oukhXi39i6xNQG7t6SvShtzvRM9cjW2B1bJXPgLHUF+6PdwhFFZa5cBKT\n"
    "4gkJxoCa5FiaowHzpTbVsGh7xG8=\n"
    "-----END CERTIFICATE-----\n";
X509List cert(TIMEAPI_CERT);
HTTPClient sender;

void setup()
{
  // put your setup code here, to run once:
  // Setup of the Serial connection for Debugging
  Serial.begin(9600);
  Serial.println("I successfully communicate!");

  pinMode(CS, OUTPUT); // Chip select pin for SPI communication
  pinMode(D0, OUTPUT); // Setup pin to toggle the HV PSU for the tubes.

  // Setup of the SPI Interface, the baud rate is fixed by the combined output
  // rise and fall time of the shift register.
  SPI.begin();
  SPI.beginTransaction(SPISettings(2500000, MSBFIRST, SPI_MODE0)); // 2,5 MHz works.

  // Create a Nixie Controller instance
  nxc = Nixie_Controller(num_nixies, digits_per_nixie);
  // Write an empty buffer to the shift registers to get them into  a defined initial state.
  send_nixie_buffer();
  nxc.reset_nixie_buf();

  // Setup of a dynamical Wifi connection
  WiFiManager wm;
  wm.setConnectTimeout(60);                 // When the ESP cannot establish a connection with the saved network within 60 seconds, it will open an access point.
  wm.setConfigPortalTimeout(180);           // The autoConnect command will stop blocking the rest of the code after 180 seconds. Probably returning false.
  wm.autoConnect("NixieClock", "12345678"); // password protected ap, this will store the last used AP in HW and reconnect on powerup. It also survives reprogrammings.

  client.setInsecure(); // TODO: Remove, we are getting secure over here ;)
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  turn_hv_on();
  t_current = millis();

  // Set time via NTP, as required for x.509 validation
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

void loop()
{
  // put your main code here, to run repeatedly:
  t_sys = millis();

  if (t_sys >= t_current + 1000)
  {
    t_current = t_sys;

    nxc.turn_on_tube(5, digit_counter / 10); // dcf_ping_counter / 10);
    nxc.turn_on_tube(6, digit_counter % 10); // dcf_ping_counter % 10);
    send_nixie_buffer();
    nxc.reset_nixie_buf();

    if (nxc.errflg_illegal_digit)
    {
      Serial.println("Illegal digit flag trigd.");
    }

    digit_counter++;
    if (digit_counter > 99)
    {
      digit_counter = 0;
    }
    printLocalTime();

    // Get local time through Wifi
    if ((WiFi.status() == WL_CONNECTED))
    {
      String https_payload_ip = https_request("https://api.ipify.org/?format=json", IPFIY_ROOT_CERT);
      String https_payload_tz = https_request("https://timeapi.io/api/time/current/zone?timeZone=Europe%2FAmsterdam", TIMEAPI_ROOT_CERT);
      Serial.println(https_payload_ip);
      JsonDocument doc;
      deserializeJson(doc, https_payload_ip);
      String ip = doc["ip"];
      Serial.println(ip);
      Serial.println(https_payload_tz);
      deserializeJson(doc, https_payload_tz);
      String hour = doc["hour"];
      Serial.println(hour);
    }
    else
    {
      Serial.println("Wifi not connected, cannot do HTTPS req.");
    }
  }
}
// put function definitions here:

void turn_hv_on()
{
  digitalWrite(D0, LOW);
  delay(1000);
}

void turn_hv_off()
{
  digitalWrite(D0, HIGH);
}

void send_nixie_buffer()
{
  if (nxc.check_buffer())
  {
    digitalWrite(CS, LOW);
    SPI.transfer(nxc.nixie_buf, num_shift_registers);
    digitalWrite(CS, HIGH);
  }
}

void printLocalTime()
{
  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  Serial.println(asctime(timeinfo));
}

String https_request(String request, const char *certificate)
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
      // HTTP header has been send and Server response header has been handled
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