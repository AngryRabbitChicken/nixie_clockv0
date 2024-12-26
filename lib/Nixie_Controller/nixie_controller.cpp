#include <Arduino.h>
#include "nixie_controller.h"

Nixie_Controller::Nixie_Controller()
: num_nixies(0), digits_per_nixie(0) {};

Nixie_Controller::Nixie_Controller(uint8_t num_nixies, uint8_t digits_per_nixie)
    : num_nixies(num_nixies), digits_per_nixie(digits_per_nixie)
{
    reset_nixie_buf();
};

void Nixie_Controller::reset_nixie_buf()
{
  for(int j = 0; j < num_shift_registers; j ++)
    nixie_buf[j] = 0;
};

void Nixie_Controller::turn_on_digit(uint8_t digit)
{
  if (digit > num_nixies * digits_per_nixie)
  {
    errflg_illegal_digit = true;
    reset_nixie_buf();
    return;
  }
  
  uint8_t j = 8 - (digit - 1) / num_shift_registers - 1;
  uint8_t i = digit - (7 - j) * num_shift_registers - 1;
  nixie_buf[j] |= (1 << i);
}

void Nixie_Controller::turn_on_tube(uint8_t tube, uint8_t digit)
{
    if(digit < 1 || digit > digits_per_nixie)
    {
      errflg_illegal_digit = true;
      reset_nixie_buf();
      return;
    }
    uint8_t digit_num = (tube - 1) * digits_per_nixie + digit;
    turn_on_digit(digit_num);
};

bool Nixie_Controller::check_buffer()
{
  uint8_t hi_bits = 0;
  for (int j = 0; j < num_shift_registers; j++)
  {
    for (int i = 0; i < 8; i++)
    {
      if ((nixie_buf[num_shift_registers - 1 - j] >> i & 1))
      {
        hi_bits ++;
      }
      if ((i + 1 + j * 8) % digits_per_nixie == 0) // Check how many bits are high, every digits_per_nixie bits.
      {
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