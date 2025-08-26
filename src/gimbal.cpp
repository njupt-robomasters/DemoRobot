#include "gimbal.hpp"
#include <Arduino.h>
#include "config.hpp"

Gimbal::Gimbal(uint32_t (&hc595_buf)[8], uint8_t pitch_pin, uint8_t shooter_pin) :
    m_hc595_buf(hc595_buf), m_pitch_pin(pitch_pin), m_shooter_pin(shooter_pin) {
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
    setShooter(false); // 关闭发射
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

void Gimbal::setShooter(bool enable) {
    for (int i = 0; i < HC595_BUF_LEN; i++)
        setBit(m_hc595_buf[i], m_shooter_pin, enable);
}

void Gimbal::onLoop() {
    float dt = m_dt.update();
    m_angle += m_angle_per_sec * dt;
    m_angle = clampf(m_angle, 90 - PITCH_ANGLE_MAX, 90 + PITCH_ANGLE_MAX);
    setAngle(m_angle);
}
