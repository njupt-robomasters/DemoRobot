#include "stepper.hpp"
#include "main.hpp"

Stepper::Stepper(uint32_t (&hc595_data)[8],
                 uint8_t en_pin, uint8_t step_pin, uint8_t dir_pin,
                 bool invert) : 
    m_hc595_data(hc595_data),
    m_en_pin(en_pin), m_step_pin(step_pin), m_dir_pin(dir_pin),
    m_invert(invert) {
    Disable();
}

void Stepper::Disable() {
    for (int i = 0; i < 8; i++)
        setBit(m_hc595_data[i], m_en_pin, 1);
}

void Stepper::Enable() {
    for (int i = 0; i < 8; i++)
        setBit(m_hc595_data[i], m_en_pin, 0);
}

void Stepper::SetFreq(float freq) {
    bool dir = (freq > 0) ^ m_invert;
    for (int i = 0; i < 8; i++)
        setBit(m_hc595_data[i], m_dir_pin, dir);
    m_arr = HC595_FREQ / abs(freq); 
}

void Stepper::SetRPM(float rpm) {
    float freq = rpm / 60 * (360 / 1.8) * STEPPER_DIVIDER;
    SetFreq(freq);
}

void Stepper::OnLoop() {
    for (int i = 0; i < 8; i++) {
        m_cnt++;
        if (m_cnt >= m_arr) {
            m_cnt = 0;
        }
        if (m_cnt < m_arr / 2) {
            setBit(m_hc595_data[i], m_step_pin, 0);
        } else {
            setBit(m_hc595_data[i], m_step_pin, 1);
        }
    }
}

void Stepper::setBit(uint32_t &data, uint8_t pos, bool val) {
    if (val) {
        // 设置位为1：使用 OR 操作
        data |= (1 << pos);
    } else {
        // 设置位为0：使用 AND 操作与取反
        data &= ~(1 << pos);
    }
}
