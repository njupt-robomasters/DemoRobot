#include "gimbal.hpp"
#include <Arduino.h>
#include "config.hpp"

Gimbal::Gimbal(uint32_t (&hc595_buf)[8], uint8_t pitch_pin, uint8_t shooter_pin) :
    m_hc595_buf(hc595_buf), m_pitch_pin(pitch_pin), m_shoot_pin(shooter_pin) {
}

void Gimbal::begin() {
    ledcSetup(0, 50, 10);
    ledcAttachPin(m_pitch_pin, 0);
    ledcWrite(0, 0); // 舵机失能
}

void Gimbal::disable() {
    // 避免重复操作
    if (m_enabled == false) return;
    m_enabled = false;
    
    ledcWrite(0, 0); // 舵机失能
    
    // 关闭发射
    continueShoot(false);
    singleShoot(false);
}

void Gimbal::enable() {
    // 避免重复操作
    if (m_enabled == true) return;
    m_enabled = true;
    
    setAngle(90); // 舵机使能后默认回中
}

void Gimbal::setAngle(float angle) {
    if (m_enabled == false) return;
    angle = clampf(angle, 0, 180);;
    uint32_t duty = mapf(angle, 0, 180, 0.5/20*1023, 2.5/20*1023);
    // Serial.println(duty);
    ledcWrite(0, duty);
}

void Gimbal::setSpeed(float angle_per_sec) {
    m_angle_per_sec = angle_per_sec;
}

void Gimbal::continueShoot(bool enable) {
    m_continue_enabled = enable;
}

void Gimbal::singleShoot(float shoot_time) {
    m_single_enabled = true;
    m_shoot_time = shoot_time;
}

void Gimbal::setInject(float freq, float power) {
    m_inject_enabled = true;
    m_arr = HC595_UPDATE_FREQ / freq;
    m_ccr = m_arr * power;
}

void Gimbal::disableInject() {
    m_inject_enabled = false;
}

void Gimbal::onLoop() {
    // 维护dt
    float dt = m_dt.update();
    
    // 云台匀速运动控制
    m_angle += m_angle_per_sec * dt;
    m_angle = clampf(m_angle, 90 - PITCH_ANGLE_MAX, 90 + PITCH_ANGLE_MAX);
    setAngle(m_angle);

    if (m_shoot_time > 0) {
        m_shoot_time -= dt;
    }

    if (m_inject_enabled) {
        // 优先音乐注入，注入期间不允许发射
        for (uint8_t i = 0; i < HC595_BUF_LEN; i++) {
            m_cnt++;
            if (m_cnt >= m_arr) m_cnt = 0;
            if (m_cnt < m_ccr) {
                setBit(m_hc595_buf[i], m_shoot_pin, true);
            } else {
                setBit(m_hc595_buf[i], m_shoot_pin, false);
            }
        }
    } else if (m_continue_enabled) {
        for (int i = 0; i < HC595_BUF_LEN; i++)
            setBit(m_hc595_buf[i], m_shoot_pin, true);
    } else if (m_single_enabled && m_shoot_time > 0) {
        for (int i = 0; i < HC595_BUF_LEN; i++)
            setBit(m_hc595_buf[i], m_shoot_pin, true);
    } else {
        for (int i = 0; i < HC595_BUF_LEN; i++)
            setBit(m_hc595_buf[i], m_shoot_pin, false);
    }
}
