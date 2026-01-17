/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_matter.h>
#include <esp_matter_console.h>
#include <esp_matter_ota.h>
#include <esp_matter_feature.h>

#include <common_macros.h>
#include <app_priv.h>
#include <app_reset.h>
#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include <platform/ESP32/OpenthreadLauncher.h>
#endif

#include <app/server/CommissioningWindowManager.h>
#include <app/server/Server.h>

// 引入必要的 Matter 组件
// #include <common/esp_matter_rainmaker.h>

static const char *TAG = "app_main";
uint16_t thermostat_endpoint_id = 0;

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

constexpr auto k_timeout_seconds = 300;

extern "C" void app_main()
{
    esp_err_t err = ESP_OK;

    esp_log_level_set("chip", ESP_LOG_WARN);
    esp_log_level_set("esp_matter_attribute", ESP_LOG_WARN);
    esp_log_level_set("esp_matter_cluster", ESP_LOG_WARN);
    esp_log_level_set("app_driver", ESP_LOG_INFO);

    /* 1. Initialize the ESP NVS layer */
    nvs_flash_init();

    /* 2. Initialize driver, defined in app_driver.cpp */
    app_driver_init();

    /* 3. Create a Matter node and add the mandatory Root Node device type on endpoint 0 */
    node::config_t node_config;
    node_t *node = node::create(&node_config, app_driver_attribute_update_cb, app_identification_cb);
    ABORT_APP_ON_FAILURE(node != nullptr, ESP_LOGE(TAG, "Failed to create Matter node"));

    /* 4. Create Thermostat Endpoint */
    thermostat::config_t thermostat_config;

    // Some default values
    thermostat_config.thermostat.local_temperature = 2500; // 25.00C

    thermostat_config.thermostat.control_sequence_of_operation = 4; // 4 = Cooling & Heating
    thermostat_config.thermostat.system_mode = 0; // Off

    thermostat_config.thermostat.feature_flags |= cluster::thermostat::feature::cooling::get_id();
    thermostat_config.thermostat.feature_flags |= cluster::thermostat::feature::heating::get_id();
    thermostat_config.thermostat.feature_flags |= cluster::thermostat::feature::auto_mode::get_id();

    // create endpoint
    endpoint_t *endpoint = thermostat::create(node, &thermostat_config, ENDPOINT_FLAG_NONE, NULL);
    ABORT_APP_ON_FAILURE(endpoint != nullptr, ESP_LOGE(TAG, "Failed to create thermostat endpoint"));

    cluster_t *cluster = cluster::get(endpoint, chip::app::Clusters::Thermostat::Id);
    esp_matter::cluster::thermostat::attribute::create_min_cool_setpoint_limit(cluster, 1600);
    esp_matter::cluster::thermostat::attribute::create_max_cool_setpoint_limit(cluster, 3000);
    esp_matter::cluster::thermostat::attribute::create_min_heat_setpoint_limit(cluster, 1600);
    esp_matter::cluster::thermostat::attribute::create_max_heat_setpoint_limit(cluster, 3000);
    esp_matter::cluster::thermostat::attribute::create_control_sequence_of_operation(cluster, 4);

    esp_matter::cluster::thermostat::attribute::create_min_setpoint_dead_band(cluster, 30);
    esp_matter::cluster::thermostat::attribute::create_thermostat_running_mode(cluster, 0);

    // save endpoint for later usage
    thermostat_endpoint_id = endpoint::get_id(endpoint);
    ESP_LOGI(TAG, "==== Thermostat created with endpoint_id %d", thermostat_endpoint_id);

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
    /* Set OpenThread platform config */
    esp_openthread_platform_config_t config = {
        .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
        .port_config = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
    };
    set_openthread_platform_config(&config);
#endif

    /* Matter start */
    err = esp_matter::start(app_event_cb);
    ABORT_APP_ON_FAILURE(err == ESP_OK, ESP_LOGE(TAG, "Failed to start Matter, err:%d", err));

#if CONFIG_ENABLE_CHIP_SHELL
    esp_matter::console::diagnostics_register_commands();
    esp_matter::console::wifi_register_commands();
    esp_matter::console::factoryreset_register_commands();
    esp_matter::console::init();
    
#endif
}
