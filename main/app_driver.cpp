/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdlib.h>
#include <string.h>

#include <esp_log.h>
#include <esp_matter.h>

#include <app_priv.h>
#include <device.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"

#include "u8g2.h"

extern "C" {
    #include "u8g2_esp32_hal.h"
}

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::identification;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

static const char *TAG = "app_driver";
extern uint16_t thermostat_endpoint_id;
u8g2_t u8g2;

void demo_u8g2()
{
    u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;

    u8g2_esp32_hal.bus.i2c.sda = I2C_SDA_PIN;
    u8g2_esp32_hal.bus.i2c.scl = I2C_SCL_PIN;
    u8g2_esp32_hal_init(u8g2_esp32_hal);

    u8g2_Setup_ssd1306_i2c_128x64_noname_f(
        &u8g2,
        U8G2_R0,
        u8g2_esp32_i2c_byte_cb,
        u8g2_esp32_gpio_and_delay_cb
    );

    u8g2_SetI2CAddress(&u8g2.u8x8, 0x78);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_ClearBuffer(&u8g2);

    u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
    u8g2_DrawStr(&u8g2, 0, 40, "Hello ESP32!");
    u8g2_SendBuffer(&u8g2);
}

// --- 硬件初始化函数 ---
esp_err_t app_driver_init()
{
    ESP_LOGI(TAG, "==== Initializing Driver...");

    /* 1. 初始化 OLED 屏幕 (I2C) - 之后填充具体代码 */
    // ssd1306_init(); 
    // ssd1306_clear_screen();
    // ssd1306_show_text("Booting...");
    ESP_LOGI(TAG, "==== OLED Display Initializing...");
    demo_u8g2();

    /* 2. 初始化 三菱 HVAC (UART) - 之后填充 */
    // mitsubishi_hvac_init();
    ESP_LOGI(TAG, "==== Mitsubishi HVAC UART Initialized (Mock)");

    return ESP_OK;
}

// --- Matter 属性更新回调 (核心逻辑) ---
// 当手机 App 修改温度、模式时，这个函数会被调用
esp_err_t app_driver_attribute_update_cb(attribute::callback_type_t type, 
                                       uint16_t endpoint_id, uint32_t cluster_id,
                                       uint32_t attribute_id, esp_matter_attr_val_t *val, 
                                       void *priv_data)
{
    if (type == PRE_UPDATE) {
        // 更新前回调，通常用于检查数值是否合法
        return ESP_OK;
    }

    // 过滤出 Thermostat Cluster 的变化
    if (cluster_id == Thermostat::Id) {
        
        // 1. 处理系统模式变化 (关/制冷/制热/自动)
        if (attribute_id == Thermostat::Attributes::SystemMode::Id) {
            uint8_t new_mode = val->val.u8;
            ESP_LOGI(TAG, "==== System Mode changed to: %d", new_mode);
            // TODO: 调用三菱控制代码: mitsubishi_set_mode(new_mode);
        }
        // 2. 处理制冷设定温度变化
        else if (attribute_id == Thermostat::Attributes::OccupiedCoolingSetpoint::Id) {
            int16_t new_temp = val->val.i16; // 单位是 0.01 度
            ESP_LOGI(TAG, "==== Cooling Setpoint changed to: %d", new_temp);
            // TODO: 调用三菱控制代码: mitsubishi_set_temp(new_temp);
        }
        // 3. 处理制热设定温度变化
        else if (attribute_id == Thermostat::Attributes::OccupiedHeatingSetpoint::Id) {
            int16_t new_temp = val->val.i16;
             ESP_LOGI(TAG, "==== Heating Setpoint changed to: %d", new_temp);
        }

        // 4. 无论什么变化，最后都刷新屏幕显示新状态
        // update_oled_display(); 
    }

    return ESP_OK;
}

// This callback is invoked when clients interact with the Identify Cluster.
// In the callback implementation, an endpoint can identify itself. (e.g., by flashing an LED or light).
esp_err_t app_identification_cb(identification::callback_type_t type,
                                        uint16_t endpoint_id, uint8_t effect_id,
                                       uint8_t effect_variant, void *priv_data)
{
    ESP_LOGI(TAG, "======== Identification callback: type: %u, effect: %u, variant: %u", type, effect_id, effect_variant);
    return ESP_OK;
}

void app_event_handle(const chip::DeviceLayer::ChipDeviceEvent *event)
{
    switch (event->Type) {
        /* 1. 网络连接状态：用于更新 OLED 的 WiFi 图标 */
        case chip::DeviceLayer::DeviceEventType::kInterfaceIpAddressChanged:
            ESP_LOGI(TAG, "@@ Network: IP Address changed (System Online)");
            // TODO: UI 显示 IP 地址
            break;

        /* 2. 蓝牙配网状态：控制 OLED 显示“等待配网”或二维码 */
        case chip::DeviceLayer::DeviceEventType::kCHIPoBLEAdvertisingChange:
            if (event->CHIPoBLEAdvertisingChange.Result == chip::DeviceLayer::kActivity_Started) {
                ESP_LOGI(TAG, "@@ BLE: Advertising started. Ready for commissioning.");
                // TODO: UI 显示 "Ready to Pair"
            } else {
                ESP_LOGI(TAG, "@@ BLE: Advertising stopped.");
            }
            break;
        /* 3. 配网完成：最关键的时刻 */
        case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
            ESP_LOGI(TAG, "@@ Matter: Commissioning successfully completed!");
            // 这里可以触发一次强制的状态上报或播放成功音效
            // TODO: UI 显示 "Pairing Success"
            // 1. 复位引脚
            gpio_reset_pin(GPIO_NUM_2); 
            // 2. 设置为输出模式
            gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
            // 3. 输出高电平（点亮）
            gpio_set_level(GPIO_NUM_2, 1);
            break;

        /* 4. 故障保护定时器触发：配网超时 */
        case chip::DeviceLayer::DeviceEventType::kFailSafeTimerExpired:
            ESP_LOGW(TAG, "@@ Matter: Commissioning failed or timed out.");
            // TODO: UI 显示 "Pairing Timeout"
            break;

        /* 5. 织网状态改变（例如设备被重置或从 App 中移除） */
        case chip::DeviceLayer::DeviceEventType::kFabricRemoved:
            ESP_LOGW(TAG, "@@ Matter: Fabric removed. Device might be factory reset.");
            // 这里可以执行清理逻辑
            break;
        default:
            // 其他琐碎事件（如 DNS 广播等）可以选择不处理
            ESP_LOGI(TAG, "@@ Other events, ignore.");
            break;
    }
}

void app_event_cb(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg)
{
    // 暂时留空，或者打印事件 ID 供调试
    ESP_LOGI(TAG, "==== System Event received: %d", event->Type);
    app_event_handle(event);
}