/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once

#include <esp_err.h>
#include <esp_matter.h>

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include "esp_openthread_types.h"
#endif

/** Default attribute values used during initialization */
#define DEFAULT_POWER true

// I2C 接口 (用于 SSD1306 OLED)
#define I2C_SDA_PIN          GPIO_NUM_21
#define I2C_SCL_PIN          GPIO_NUM_22

// UART 接口 (用于三菱 CN105)
#define HVAC_UART_TX_PIN     GPIO_NUM_17
#define HVAC_UART_RX_PIN     GPIO_NUM_16
#define HVAC_UART_PORT       UART_NUM_1

typedef void *app_driver_handle_t;

esp_err_t app_driver_init();

esp_err_t app_driver_attribute_update_cb(esp_matter::attribute::callback_type_t type, 
                                       uint16_t endpoint_id, uint32_t cluster_id,
                                       uint32_t attribute_id, esp_matter_attr_val_t *val, 
                                       void *priv_data);

esp_err_t app_identification_cb(esp_matter::identification::callback_type_t type,
                                        uint16_t endpoint_id, uint8_t effect_id,
                                       uint8_t effect_variant, void *priv_data);

void app_event_cb(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);


#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#define ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG()                                           \
    {                                                                                   \
        .radio_mode = RADIO_MODE_NATIVE,                                                \
    }

#define ESP_OPENTHREAD_DEFAULT_HOST_CONFIG()                                            \
    {                                                                                   \
        .host_connection_mode = HOST_CONNECTION_MODE_NONE,                              \
    }

#define ESP_OPENTHREAD_DEFAULT_PORT_CONFIG()                                            \
    {                                                                                   \
        .storage_partition_name = "nvs", .netif_queue_size = 10, .task_queue_size = 10, \
    }
#endif
