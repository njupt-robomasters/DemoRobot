#pragma once

#include <cstdint>
#include "utils.hpp"

class Gimbal {
public:
    Gimbal(uint32_t (&hc595_buf)[8], uint8_t pitch_pin, uint8_t shooter_pin);

    void begin();

    void disable();
    void enable();

    void setAngle(float angle);
    void setSpeed(float angle_per_sec);

    void setShooter(bool enable);

    void onLoop();

private:
    uint32_t (&m_hc595_buf)[8];
    const uint8_t m_pitch_pin, m_shooter_pin;

    bool m_enabled = false;

    DT m_dt;
    float m_angle = 90;
    float m_angle_per_sec = 0;
};
