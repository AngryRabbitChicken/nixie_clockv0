#include <Arduino.h>
#include <SPI.h>

// put function declarations here:
// TODO: The transfer only works with an array of 8 Bit integers, therefore wirte the check for an array of 8 bit integers.
// each integer then corrseponds to one shift reg. Trying a little change.
bool legal_spi_value(uint64_t);
void print_information(uint8_t *);

uint32_t t_current, t_sys;
uint64_t spi_value = 1;

uint8_t nixie_buf[8];

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("I successfully communicate!");
  t_current = millis();
  Serial.println(sizeof(nixie_buf));
  print_information(nixie_buf);
}

void loop()
{
  // put your main code here, to run repeatedly:
  t_sys = millis();

  if (t_sys >= t_current + 1000)
  {
    /* t_current = t_sys;
    if(legal_spi_value(spi_value, 10, 6)) Serial.println("Detected a legal SPI value!");
    else Serial.println("Detected an illegal SPI value!"); */
  }
}

// put function definitions here:

bool legal_spi_value(uint64_t value, uint8_t tube_digits, uint8_t num_tubes)
{
  uint8_t bits_on = 0;

  for (int j = 0; j < num_tubes; j++)
  {

    for (int i = 0; i < tube_digits; i++)
    {

      if ((value >> (i + j * tube_digits) & 1))
        bits_on++;
    }
    Serial.println(bits_on);
    if (bits_on > 1)
      return false;
    bits_on = 0;
  }
  return true;
}

void print_information(uint8_t *arr)
{
  uint8_t size = *(&arr + 1) - arr;
  ;
  Serial.println(size);
}