#include "main.h"
#include "can-utils.h"
#include "wifi-utils.h"
#include "watchdog-utils.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(Main, LOG_LEVEL_DBG);

int main()
{
    int ret;

    ret = setup_watchdog();
    if (ret < 0) {
        LOG_ERR("Watchdog setup failed");
        return 1;
    }

    init_wifi();

    ret = setup_can_device();
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
                k_sleep(K_SECONDS(WAIT_ON_ERROR_TIME));
            }

            continue;
        }

        if (!atomic_get(&mqtt_ready)) {
            LOG_INF("MQTT setup in progress...");

            ret = setup_mqtt();
            if (ret < 0) {
                LOG_ERR("MQTT setup failed");
                k_sleep(K_SECONDS(WAIT_ON_ERROR_TIME));
            }

            continue;
        }

        ret = poll_mqtt();
        if (ret != 0) {
            LOG_ERR("Failed to poll MQTT socket, error %d", ret);
            k_sleep(K_SECONDS(WAIT_ON_ERROR_TIME));
            continue;
        }

        ret = feed_watchdog();
        if (ret != 0) {
            LOG_ERR("Failed to feed watchdog, error %d", ret);
            k_sleep(K_SECONDS(WAIT_ON_ERROR_TIME));
            continue;
        } else {
            LOG_DBG("Watchdog fed");
        }
    }

    return 0;
}
