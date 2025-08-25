#include "main.hpp"
#include <Arduino.h>
#include <driver/i2s.h>
#include <XboxSeriesXControllerESP32_asukiaaa.hpp>

#include "stepper.hpp"
#include "chassis.hpp"

// 需要在此替换成自己的手柄蓝牙MAC地址
XboxSeriesXControllerESP32_asukiaaa::Core
    // xbox("59:9b:52:75:33:2f");
    xbox("40:8e:2c:92:72:aa");

uint32_t hc595_buf[HC595_BUF_LEN];

Stepper s1(hc595_buf, X_EN, X_STEP, X_DIR);
Stepper s2(hc595_buf, Y_EN, Y_STEP, Y_DIR);
Stepper s3(hc595_buf, Z_EN, Z_STEP, Z_DIR);
Stepper s4(hc595_buf, E0_EN, E0_STEP, E0_DIR);
Chassis chassis(s1, s2, s3, s4);

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

void xbox_task(void *argument) {
    while (1) {
        xbox.onLoop();
        if (xbox.isConnected() && !xbox.isWaitingForFirstNotification()) {
            // Serial.println(xbox.xboxNotif.toString());

            float RX = (float) xbox.xboxNotif.joyRHori / 65535 * 2 - 1;
            if (abs(RX) < XBOX_DEADLINE) RX = 0;
            float RY = -((float) xbox.xboxNotif.joyRVert / 65535 * 2 - 1);
            if (abs(RY) < XBOX_DEADLINE) RY = 0;
            float LX = (float) xbox.xboxNotif.joyLHori / 65535 * 2 - 1;
            if (abs(LX) < XBOX_DEADLINE) LX = 0;
            float LY = -((float) xbox.xboxNotif.joyLVert / 65535 * 2 - 1);
            if (abs(LY) < XBOX_DEADLINE) LY = 0;
            // Serial.println(String(RX) + " " + String(RY) + " " + String(LX) + " " + String(LY));

            float vx = RX * VXY_MAX;
            float vy = RY * VXY_MAX;
            float vrpm = -LX * VRPM_MAX;
            chassis.Enable();
            chassis.SetSpeed(vx, vy, vrpm);
        } else {
            chassis.Disable();
        }
        delay(1); // 防止触发任务看门狗
    }
}

void setup() {
    Serial.begin(115200);
    
    init_i2s();
    xbox.begin();

    xTaskCreatePinnedToCore(xbox_task, "xbox_task", 4096, NULL, 1, NULL, 0); // 在核心0上运行
}

void loop() {
    // loop默认在核心1上运行
    chassis.OnLoop();
    size_t bytes_written;
    i2s_write(I2S_NUM_0, hc595_buf, sizeof(hc595_buf), &bytes_written, portMAX_DELAY);
}
