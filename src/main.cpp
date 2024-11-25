#include <Arduino.h>
#include <SPI.h>
#include <WiFiManager.h>

// put function declarations here:
// TODO: The transfer only works with an array of 8 Bit integers, therefore wirte the check for an array of 8 bit integers.
// each integer then corrseponds to one shift reg. Trying a little change.
bool legal_spi_transmission(uint8_t *);
void print_information(uint8_t *);

uint32_t t_current, t_sys;

const uint8_t num_shift_regs = 8;
const uint8_t digits_per_nixie = 10;
uint8_t nixie_buf[num_shift_regs];

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
  res = wm.autoConnect("NixieClock", "12345678"); // password protected ap

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

      if ((i + 1 + j * 8) % digits_per_nixie == 0) // Check how many bits are high, every 10 bits.
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

void print_arr_size(uint8_t *arr)
{
  uint8_t size = *(&arr + 1) - arr;
  Serial.println(size);
}