#include "main.h"
#include "can-utils.h"
#include "wifi-utils.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/can.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(Gateway, LOG_LEVEL_DBG);

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static const struct device *can_dev = DEVICE_DT_GET(DT_ALIAS(can0));

int main()
{
    if (!device_is_ready(led.port)) {
        LOG_ERR("LED device not ready");
        return 1;
    }

    if (!device_is_ready(can_dev)) {
        LOG_ERR("CAN device not ready");
        return 1;
    }

    int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure LED as output, error %d", ret);
        return 1;
    }

    ret = setup_can_device(can_dev);
    if (ret < 0) {
        LOG_ERR("Failed to setup can device, error %d", ret);
        return 1;
    }

    LOG_INF("Setup complete, enter main processing loop");

    while (true) {
        if (!atomic_get(&wifi_ready)) {
            LOG_INF("WiFi setup in progress...");

            ret = setup_wifi();
            if (ret < 0) {
                LOG_ERR("WiFi connect failed");
            }

            continue;
        }

        if (!atomic_get(&mqtt_ready)) {
            LOG_INF("MQTT setup in progress...");

            ret = setup_mqtt();
            if (ret < 0) {
                LOG_ERR("MQTT setup failed");
            }

            continue;
        }

        k_sem_take(&mqtt_msg_app_received, K_FOREVER);
        LOG_INF("Msg received");
    }

    return 0;
}
