#pragma once

#include <cstdint>
#include "utils.hpp"

class Gimbal {
public:
    Gimbal(uint32_t (&hc595_buf)[8], uint8_t pitch_pin, uint8_t shoot_pin);

    void begin();

    void disable();
    void enable();

    void setAngle(float angle);
    void setSpeed(float angle_per_sec);

    void continueShoot(bool enable);
    void singleShoot(float shoot_time);

    void setInject(float freq, float power);
    void disableInject();

    void onLoop();

private:
    uint32_t (&m_hc595_buf)[8];
    const uint8_t m_pitch_pin, m_shoot_pin;

    bool m_enabled = false;

    // 用于pitch匀速运动
    DT m_dt;
    float m_angle = 90;
    float m_angle_per_sec = 0;

    // 用于发弹
    bool m_continue_enabled = false, m_single_enabled = false;
    float m_shoot_time = 0;

    // 用于音乐注入
    bool m_inject_enabled = false;
    uint32_t m_cnt, m_arr = 0, m_ccr = 0;
};
