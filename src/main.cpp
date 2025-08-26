#include <Arduino.h>
#include <driver/i2s.h>
#include <XboxSeriesXControllerESP32_asukiaaa.hpp>

#include "config.hpp"
#include "utils.hpp"
#include "stepper.hpp"
#include "chassis.hpp"
#include "gimbal.hpp"
#include "music.hpp"

// 需要在此替换成自己的手柄蓝牙MAC地址
XboxSeriesXControllerESP32_asukiaaa::Core
    // xbox("59:9b:52:75:33:2f");
    xbox("40:8e:2c:92:72:aa");

// 74HC595
uint32_t hc595_buf[HC595_BUF_LEN];

// 底盘
Stepper s1(hc595_buf, X_EN, X_STEP, X_DIR);
Stepper s2(hc595_buf, Y_EN, Y_STEP, Y_DIR);
Stepper s3(hc595_buf, Z_EN, Z_STEP, Z_DIR);
Stepper s4(hc595_buf, E0_EN, E0_STEP, E0_DIR);
Chassis chassis(s1, s2, s3, s4);

// 云台
Gimbal gimbal(hc595_buf, PITCH_PIN, H_BED);

TaskHandle_t music_task_handle = NULL;

void init_i2s() {
    i2s_config_t i2s_config = {
        .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = HC595_UPDATE_FREQ * 2,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_STAND_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 2,
        .dma_buf_len = HC595_BUF_LEN
    };
    i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = HC595_CLK,
        .ws_io_num = HC595_LATCH,
        .data_out_num = HC595_DATA,
        .data_in_num = I2S_PIN_NO_CHANGE,
    };
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    i2s_start(I2S_NUM_0);
}

void music_task(void *argument) {
    // 过滤掉开头的空白音符
    int i = 0;
    for (; i < sizeof(freqs)/sizeof(freqs[0]); i++) {
        if (freqs[i] != 0) 
            break;
    }
    for (; i < sizeof(freqs)/sizeof(freqs[0]); i++) {
        if (freqs[i] == 0) {
            gimbal.disableInject();
            vTaskDelay(durations[i]);
        } else {
            gimbal.setInject(freqs[i], MUSIC_POWER);
            vTaskDelay(durations[i]);

            // 在音符之间添加10ms间隔，防止相同音符连音
            gimbal.disableInject();
            vTaskDelay(10);
        }
    }
    music_task_handle = NULL;
    vTaskDelete(NULL);
}

void xbox_task(void *argument) {
    while (1) {
        xbox.onLoop();
        if (xbox.isConnected() && !xbox.isWaitingForFirstNotification()) {
            // Serial.println(xbox.xboxNotif.toString());

            // 手柄已连接，使能底盘和云台
            chassis.enable();
            gimbal.enable();

            // 读取左右摇杆值，归一化到-1~1范围内
            float LX = mapf(xbox.xboxNotif.joyLHori, 0, 65535, -1, 1);
            if (abs(LX) < XBOX_DEADLINE) LX = 0; // 用于解决遥感零偏
            float LY = mapf(xbox.xboxNotif.joyLVert, 0, 65535, 1, -1);
            if (abs(LY) < XBOX_DEADLINE) LY = 0;
            float RX = mapf(xbox.xboxNotif.joyRHori, 0, 65535, -1, 1);
            if (abs(RX) < XBOX_DEADLINE) RX = 0;
            float RY = mapf(xbox.xboxNotif.joyRVert, 0, 65535, 1, -1);
            if (abs(RY) < XBOX_DEADLINE) RY = 0;
            // 左摇杆制底盘前后左右平移
            // 右摇杆制底盘旋转和云台俯仰
            float vx = LX * VXY_MAX;
            float vy = LY * VXY_MAX;
            float vr = -RX * VR_MAX;
            chassis.setSpeed(vx, vy, vr);
            gimbal.setSpeed(RY * PITCH_SPEED_MAX);

            // 读取扳机值，归一化到0~1范围内
            float LT = mapf(xbox.xboxNotif.trigLT, 0, 1023, 0, 1);
            float RT = mapf(xbox.xboxNotif.trigRT, 0, 1023, 0, 1);
            // 扳机控制云台开火（左右任意扳机按下即开火）
            if (LT > XBOX_DEADLINE || RT > XBOX_DEADLINE) {
                gimbal.continueShoot(true);
            } else {
                gimbal.continueShoot(false);
            }

            // 扳机上方的按键控制单发
            static bool btnLRB_last = false;
            bool btnLRB = xbox.xboxNotif.btnLB || xbox.xboxNotif.btnRB;
            if (btnLRB && btnLRB != btnLRB_last) {
                gimbal.singleShoot(SINGLE_SHOOT_TIME);
            }
            btnLRB_last = btnLRB;

            static bool btnB_last = false;
            static bool btnX_last = false;
            // 按X播放音乐
            if (xbox.xboxNotif.btnX && xbox.xboxNotif.btnX != btnX_last) {
                if (music_task_handle == NULL) {
                    xTaskCreatePinnedToCore(music_task, "music_task", 4096, NULL, 1, &music_task_handle, 0);
                }
            }
            // 按B停止音乐
            if (xbox.xboxNotif.btnB && xbox.xboxNotif.btnB != btnB_last) {
                if (music_task_handle != NULL) {
                    vTaskDelete(music_task_handle);
                    music_task_handle = NULL;
                    gimbal.disableInject();
                }
            }
            btnB_last = xbox.xboxNotif.btnB;
            btnX_last = xbox.xboxNotif.btnX;
        
        } else {
            // 手柄未连接，底盘和云台失能
            chassis.disable();
            gimbal.disable();
            // 停止音乐
            if (music_task_handle != NULL) {
                vTaskDelete(music_task_handle);
                music_task_handle = NULL;
                gimbal.disableInject();
            }
        }
        delay(1); // 防止饿死任务看门狗
    }
}

void setup() {
    Serial.begin(115200); // 初始化串口，波特率115200

    init_i2s(); // 初始化I2S，用于驱动74HC595

    gimbal.begin(); // 初始化云台

    xbox.begin(); // 初始化Xbox手柄

    xTaskCreatePinnedToCore(xbox_task, "xbox_task", 4096, NULL, 1, NULL, 0); // 创建Xbox手柄任务，绑定到核心0
}

void loop() {
    // loop默认在核心1上运行
    // 不建议在核心1上运行其他代码，以免影响74HC595刷新
    chassis.onLoop();
    gimbal.onLoop();
    size_t bytes_written;
    i2s_write(I2S_NUM_0, hc595_buf, sizeof(hc595_buf), &bytes_written, portMAX_DELAY);
}
