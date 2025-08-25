#include "chassis.hpp"
#include "main.hpp"

Chassis::Chassis(Stepper &s1, Stepper &s2, Stepper &s3, Stepper &s4) :
    m_s1(s1), m_s2(s2), m_s3(s3), m_s4(s4) {
}

void Chassis::Disable() {
    // 应用到每一个轮子
    m_s1.Disable();
    m_s2.Disable();
    m_s3.Disable();
    m_s4.Disable();
}

void Chassis::Enable() {
    // 应用到每一个轮子
    m_s1.Enable();
    m_s2.Enable();
    m_s3.Enable();
    m_s4.Enable();
}

void Chassis::SetSpeed(float vx, float vy, float vrpm) {
    m_vx_target = vx;
    m_vy_target = vy;
    m_vrpm_target = vrpm;
}

void Chassis::OnLoop() {
    // 缓加速
    updateDT();
    applyAcceleration(m_vx, m_vx_target, AXY);
    applyAcceleration(m_vy, m_vy_target, AXY);
    applyAcceleration(m_vrpm, m_vrpm_target, ARPM);

    // 底盘旋转角速度（单位：rpm） -> 底盘旋转线速度（单位：m/s）
    float vz = m_vrpm / 60 * (2 * M_PI * CHASSIS_RADIUS);

    // 底盘运动学解算（全部为标准单位：m/s）
    float s1 = sqrtf(0.5f) * (-m_vx + m_vy) + vz;
    float s2 = sqrtf(0.5f) * (-m_vx - m_vy) + vz;
    float s3 = sqrtf(0.5f) * (m_vx - m_vy) + vz;
    float s4 = sqrtf(0.5f) * (m_vx + m_vy) + vz;

    // 设置轮电机速度
    m_s1.SetRPM(s1 / (2 * M_PI * WHEEL_RADIUS) * 60);
    m_s2.SetRPM(s2 / (2 * M_PI * WHEEL_RADIUS) * 60);
    m_s3.SetRPM(s3 / (2 * M_PI * WHEEL_RADIUS) * 60);
    m_s4.SetRPM(s4 / (2 * M_PI * WHEEL_RADIUS) * 60);

    // 更新轮电机
    m_s1.OnLoop();
    m_s2.OnLoop();
    m_s3.OnLoop();
    m_s4.OnLoop();
}

void Chassis::updateDT() {
    uint32_t now_micros = micros();
    m_dt = (now_micros - m_last_micros) / 1e6f;
    m_last_micros = now_micros;
}

void Chassis::applyAcceleration(float &v, float v_target, float a) {
    if (v_target > v) {
        v += a * m_dt;
        if (v > v_target) v = v_target;
    } else {
        v -= a * m_dt;
        if (v < v_target) v = v_target;
    }
}
