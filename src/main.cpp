#include <Arduino.h>
#include <SPI.h>

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
  Serial.begin(9600);
  Serial.println("I successfully communicate!");

  nixie_buf[0] = 0b00000000;
  nixie_buf[1] = 0b00000110;
  nixie_buf[2] = 0b00000000;
  nixie_buf[3] = 0b00000000;
  nixie_buf[4] = 0b00000001;
  nixie_buf[5] = 0b00000000;
  nixie_buf[6] = 0b00000001;
  nixie_buf[7] = 0b00000100;

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

      if ((i + 1 + j * 8) % 10 == 0)
      {
        Serial.printf("I am at position %d.\n", i + 1 + j * 8);
        if (hi_bits > 1)
        {
          return false;
        }
        hi_bits = 0;
      }
    }
  }
  return true;
}

void print_information(uint8_t *arr)
{
  uint8_t size = *(&arr + 1) - arr;
  Serial.println(size);
}