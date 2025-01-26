#include <Arduino.h>
#include <SPI.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <time.h>

#include "Nixie_Controller.h"

#define CS D8 //The chip select pin connected to the shift regs is pin D8.

// put function declarations here:

//turn on and off the high voltage PSU for the nixies
void turn_hv_on();
void turn_hv_off();

//Check the buffer for integrity and send it.
void send_nixie_buffer();

//Print the NTP time
void printLocalTime();

// general
uint32_t t_current, t_sys;

// Nixie control related
const uint8_t num_nixies = 6;
const uint8_t digits_per_nixie = 10;
volatile uint8_t digit_counter = 0; //Remove later, this just for testing
Nixie_Controller nxc;

// Time request related
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600 * 1;
const int   daylightOffset_sec = 3600 * 0;

void setup()
{
  // put your setup code here, to run once:
  // Setup of the Serial connection for Debugging
  Serial.begin(9600);
  Serial.println("I successfully communicate!");

  pinMode(CS, OUTPUT); //Chip select pin for SPI communication
  pinMode(D0, OUTPUT); //Setup pin to toggle the HV PSU for the tubes.

  // Setup of the SPI Interface, the baud rate is fixed by the combined output
  // rise and fall time of the shift register.  
  SPI.begin();
  SPI.beginTransaction(SPISettings(2500000, MSBFIRST, SPI_MODE0)); //2,5 MHz works.
  //Create a Nixie Controller instance
  nxc = Nixie_Controller(num_nixies, digits_per_nixie);
  //Write an empty buffer to the shift registers to get them into  a defined initial state.
  send_nixie_buffer();
  nxc.reset_nixie_buf();
  

  //Setup of a dynamical Wifi connection
  WiFiManager wm;
  wm.setConnectTimeout(60);                       // When the ESP cannot establish a connection with the saved network within 60 seconds, it will open an access point.
  wm.setConfigPortalTimeout(180);                 // The autoConnect command will stop blocking the rest of the code after 180 seconds. Probably returning false.
  wm.autoConnect("NixieClock", "12345678"); // password protected ap, this will store the last used AP in HW and reconnect on powerup. It also survives reprogrammings.
 
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  turn_hv_on();
  t_current = millis();
}

void loop()
{
  // put your main code here, to run repeatedly:  
  t_sys =  millis();

  if (t_sys >= t_current + 1000)
  {
    t_current = t_sys;

    nxc.turn_on_tube(5, digit_counter / 10);//dcf_ping_counter / 10);
    nxc.turn_on_tube(6, digit_counter % 10);//dcf_ping_counter % 10);
    send_nixie_buffer();
    nxc.reset_nixie_buf();
      
    if (nxc.errflg_illegal_digit)
    {
      Serial.println("Illegal digit flag trigd.");
    }

    digit_counter ++;
    if (digit_counter > 99)
    {
      digit_counter = 0;
    }
    
    printLocalTime();
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
  struct tm * timeinfo;
  time (&rawtime);
  timeinfo = localtime (&rawtime);
  Serial.println(asctime(timeinfo));
}