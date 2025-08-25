#pragma once

#include <Arduino.h>

class Stepper { 
public:
    Stepper(uint32_t (&hc595_buf)[8],
            uint8_t en_pin, uint8_t step_pin, uint8_t dir_pin,
            bool invert=false);

    void Disable();
    void Enable();

    void SetFreq(float freq);
    void SetRPM(float rpm);

    void OnLoop();

private:
    uint32_t (&m_hc595_buf)[8];
    const uint8_t m_en_pin, m_dir_pin, m_step_pin;
    const bool m_invert;

    uint32_t m_cnt = 0, m_arr = 0;

    void setBit(uint32_t &data, uint8_t pos, bool val);
};
