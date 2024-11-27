#include <Arduino.h>
#include <SPI.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

// put function declarations here:
// TODO: The transfer only works with an array of 8 Bit integers, therefore wirte the check for an array of 8 bit integers.
// each integer then corrseponds to one shift reg. Trying a little change.
bool legal_spi_transmission(uint8_t *);
void print_information(uint8_t *);
void startUDP();
void sendNTPpacket(IPAddress &);
uint32_t getTime();

// general
uint32_t t_current, t_sys;

// Nixie control related
const uint8_t num_shift_regs = 8;
const uint8_t digits_per_nixie = 10;
uint8_t nixie_buf[num_shift_regs];

// UDP related
WiFiUDP UDP;
IPAddress timeServerIP; // time.nist.gov NTP server address
const char *NTPServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48;  // NTP time stamp is in the first 48 bytes of the message
byte NTPBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing packets

void setup()
{
  // put your setup code here, to run once:
  // Setup of the Serial connection for Debugging
  Serial.begin(9600);
  Serial.println("I successfully communicate!");

  // Setup of a buffer storing for an exemplary Nixie Output configuration
  nixie_buf[0] = 0b00000000;
  nixie_buf[1] = 0b00000110;
  nixie_buf[2] = 0b00000000;
  nixie_buf[3] = 0b00000000;
  nixie_buf[4] = 0b00000001;
  nixie_buf[5] = 0b00000000;
  nixie_buf[6] = 0b00000001;
  nixie_buf[7] = 0b00000100;

  // Setup of the SPI Interface, the baud rate is fixed by the combined output
  // rise and fall time of the shift register.
  SPI.begin();
  SPI.beginTransaction(SPISettings(2500000, MSBFIRST, SPI_MODE0));

  // Setup of a dynamical Wifi connection
  WiFiManager wm;
  bool res;
  wm.setConnectTimeout(60);                       // When the ESP cannot establish a connection with the saved network within 60 seconds, it will open an access point.
  wm.setConfigPortalTimeout(180);                 // The autoConnect command will stop blocking the rest of the code after 180 seconds. Probably returning false.
  res = wm.autoConnect("NixieClock", "12345678"); // password protected ap, this will store the last used AP in HW and reconnect on powerup. It also survives reprogrammings.

  if (!res)
  {
    Serial.println("Failed to connect");
    // ESP.restart();
  }
  else
  {
    // if you get here you have connected to the WiFi
    Serial.println("connected...yaay :)");
  }

  if (WiFi.status() == WL_CONNECTED) // This one is just a test whether the WiFi handle is attached to the connection created by the WiFi manager.
  {
    Serial.println("Wifi confirms the connection.");
  }

  startUDP();

  if (!WiFi.hostByName(NTPServerName, timeServerIP))
  { // Get the IP address of the NTP server
    Serial.println("DNS lookup failed. Rebooting.");
    Serial.flush();
    ESP.reset();
  }
  Serial.print("Time server IP:\t");
  Serial.println(timeServerIP);

  Serial.println("\r\nSending NTP request ...");
  sendNTPpacket(timeServerIP);

  t_current = millis();
}

void loop()
{
  // put your main code here, to run repeatedly:
  t_sys = millis();

  if (t_sys >= t_current + 1000)
  {
    t_current = t_sys;
    if (legal_spi_transmission(nixie_buf))
      Serial.println("Detected a legal SPI value!");
    else
      Serial.println("Detected an illegal SPI value!");

    SPI.transfer(nixie_buf, num_shift_regs);

    Serial.println("\r\nSending NTP request ...");
    sendNTPpacket(timeServerIP);
    Serial.println(getTime());
  }
}

// put function definitions here:

bool legal_spi_transmission(uint8_t spi_buf[num_shift_regs])
{
  uint8_t hi_bits = 0;
  for (int j = 0; j < num_shift_regs; j++)
  {
    for (int i = 0; i < 8; i++)
    {
      if ((spi_buf[j] >> i & 1))
      {
        hi_bits++;
      }

      if ((i + 1 + j * 8) % digits_per_nixie == 0) // Check how many bits are high, every digits_per_nixie bits.
      {
        Serial.printf("I am at position %d.\n", i + 1 + j * 8);
        if (hi_bits > 1)
        {
          return false; // If more then one bit is high in digits_per_nixie bits, then two numbers would light up at the same time, which we don't want.
        }
        hi_bits = 0;
      }
    }
  }
  return true;
}

void startUDP()
{
  Serial.println("Starting UDP");
  UDP.begin(123); // Start listening for UDP messages on port 123
  Serial.print("Local port:\t");
  Serial.println(UDP.localPort());
  Serial.println();
}

void sendNTPpacket(IPAddress &address)
{
  memset(NTPBuffer, 0, NTP_PACKET_SIZE); // set all bytes in the buffer to 0
  // Initialize values needed to form NTP request
  NTPBuffer[0] = 0b11100011; // LI, Version, Mode
  // send a packet requesting a timestamp:
  UDP.beginPacket(address, 123); // NTP requests are to port 123
  UDP.write(NTPBuffer, NTP_PACKET_SIZE);
  UDP.endPacket();
}

uint32_t getTime()
{
  if (UDP.parsePacket() == 0)
  { // If there's no response (yet)
    return 0;
  }
  UDP.read(NTPBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
  // Combine the 4 timestamp bytes into one 32-bit number
  uint32_t NTPTime = (NTPBuffer[40] << 24) | (NTPBuffer[41] << 16) | (NTPBuffer[42] << 8) | NTPBuffer[43];
  // Convert NTP time to a UNIX timestamp:
  // Unix time starts on Jan 1 1970. That's 2208988800 seconds in NTP time:
  const uint32_t seventyYears = 2208988800UL;
  // subtract seventy years:
  uint32_t UNIXTime = NTPTime - seventyYears;

  return UNIXTime;
}

void print_arr_size(uint8_t *arr)
{
  uint8_t size = *(&arr + 1) - arr;
  Serial.println(size);
}