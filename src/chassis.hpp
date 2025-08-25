#pragma once

#include <Arduino.h>
#include "stepper.hpp"

class Chassis {
public:
    Chassis(Stepper &s1, Stepper &s2, Stepper &s3, Stepper &s4);

    void Disable();
    void Enable();

    void SetSpeed(float vx, float vy, float vrpm);

    void OnLoop();

private:
    Stepper& m_s1, m_s2, m_s3, m_s4;

    uint32_t m_last_micros = 0;
    float m_dt = 0;

    float m_vx_target = 0, m_vy_target = 0, m_vrpm_target = 0;
    float m_vx = 0, m_vy = 0, m_vrpm = 0;

    void updateDT();
    void applyAcceleration(float &v, float v_target, float a);
};
