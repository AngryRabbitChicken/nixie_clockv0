#include <Arduino.h>

#ifndef nixie_controller
#define nixie_controller

const uint8_t num_shift_registers = 8;

class Nixie_Controller
{       
    public:
        uint8_t num_nixies;
        uint8_t digits_per_nixie;
        uint8_t nixie_buf[num_shift_registers];
        
        bool errflg_illegal_digit = false;

        Nixie_Controller();
        Nixie_Controller(uint8_t num_nixies, uint8_t digits_per_nixie);

        void reset_nixie_buf();
        void turn_on_digit(uint8_t digit);
        void turn_on_tube(uint8_t tube, uint8_t digit);
        bool check_buffer();
};

#endif
