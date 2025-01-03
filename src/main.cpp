#include <Arduino.h>
#include <SPI.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include "Nixie_Controller.h"

#define CS D8 //The chip select pin connected to the shift regs is pin D8.

// put function declarations here:

//turn on and off the high voltage PSU for the nixies
void turn_hv_on();
void turn_hv_off();

//Check the buffer for integrity and send it.
void send_nixie_buffer();

//UDP stuff?
void print_information(uint8_t *);
void startUDP();
void sendNTPpacket(IPAddress &);
uint32_t getTime();

// general
uint32_t t_current, t_sys;

// Nixie control related
const uint8_t num_nixies = 6;
const uint8_t digits_per_nixie = 10;
uint8_t digit_counter = 1; //Remove later, this just for testing
Nixie_Controller nxc;

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



  // Setup of the SPI Interface, the baud rate is fixed by the combined output
  // rise and fall time of the shift register.
  pinMode(CS, OUTPUT); //Chip select pin for SPI communication
  SPI.begin();
  SPI.beginTransaction(SPISettings(2500000, MSBFIRST, SPI_MODE0));

  //Create a Nixie Controller instance
  nxc = Nixie_Controller(num_nixies, digits_per_nixie);

  //Write an empty buffer to the shift registers to get them into  a defined initial state.
  send_nixie_buffer();
  nxc.reset_nixie_buf();
  

  // Setup of a dynamical Wifi connection
  // WiFiManager wm;
  // bool res;
  // wm.setConnectTimeout(60);                       // When the ESP cannot establish a connection with the saved network within 60 seconds, it will open an access point.
  // wm.setConfigPortalTimeout(180);                 // The autoConnect command will stop blocking the rest of the code after 180 seconds. Probably returning false.
  // res = wm.autoConnect("NixieClock", "12345678"); // password protected ap, this will store the last used AP in HW and reconnect on powerup. It also survives reprogrammings.

  // if (!res)
  // {
  //   Serial.println("Failed to connect");
  //   // ESP.restart();
  // }
  // else
  // {
  //   // if you get here you have connected to the WiFi
  //   Serial.println("connected...yaay :)");
  // }

  // if (WiFi.status() == WL_CONNECTED) // This one is just a test whether the WiFi handle is attached to the connection created by the WiFi manager.
  // {
  //   Serial.println("Wifi confirms the connection.");
  // }

  // startUDP();

  // if (!WiFi.hostByName(NTPServerName, timeServerIP))
  // { // Get the IP address of the NTP server
  //   Serial.println("DNS lookup failed. Rebooting.");
  //   Serial.flush();
  //   ESP.reset();
  // }
  // Serial.print("Time server IP:\t");
  // Serial.println(timeServerIP);

  // Serial.println("\r\nSending NTP request ...");
  // sendNTPpacket(timeServerIP);

  //Finally, set pin D0 to output and turn on the tubes.
  pinMode(D0, OUTPUT);
  turn_hv_on();
  delay(1000);
  t_current = millis();
}

void loop()
{
  // put your main code here, to run repeatedly:
  t_sys = millis();

  if (t_sys >= t_current + 1000)
  {
    t_current = t_sys;
    
    for(int j = 0; j < num_shift_registers; j++)
      Serial.println(nxc.nixie_buf[j], BIN);    
    
    if (nxc.check_buffer())
    {
      Serial.println("Detected a legal SPI value!");
      send_nixie_buffer(); //keep in mind, this checks for the nixie buffer integrity already. The if-else this is in is only for demo purpose.       
    }
    else
    {
      Serial.println("Detected an illegal SPI value!");
    }
     
    nxc.reset_nixie_buf();
    nxc.turn_on_tube(1, digit_counter);
    nxc.turn_on_tube(2, 11 - digit_counter);
    nxc.turn_on_tube(3, digit_counter);
    nxc.turn_on_tube(4, 11 - digit_counter);
    nxc.turn_on_tube(5, digit_counter);
    nxc.turn_on_tube(6, 11 - digit_counter);
    if (digit_counter == 10)
    {
      digit_counter = 0;
    }
    digit_counter ++;
  }
}

// put function definitions here:

void turn_hv_on()
{
  digitalWrite(D0, LOW);
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