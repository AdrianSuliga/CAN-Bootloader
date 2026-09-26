#include "main.h"
#include "can-utils.h"
#include "wifi-utils.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(Gateway, LOG_LEVEL_DBG);

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct device *wdt = DEVICE_DT_GET(DT_ALIAS(watchdog0));

static int setup_watchdog(const struct device *wdt);

int main()
{
    if (!device_is_ready(led.port)) {
        LOG_ERR("LED device not ready");
        return 1;
    }

    int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure LED as output, error %d", ret);
        return 1;
    }

    if (!device_is_ready(wdt)) {
        LOG_ERR("Watchdog not ready");
        return 1;
    }

    ret = setup_watchdog(wdt);
    if (ret < 0) {
        LOG_ERR("Watchdog setup failed");
        return 1;
    }

    int wdt_channel_id = ret;

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

        ret = wdt_feed(wdt, wdt_channel_id);
        if (ret != 0) {
            LOG_ERR("Failed to feed watchdog, error %d", ret);
            k_sleep(K_SECONDS(WAIT_ON_ERROR_TIME));
            continue;
        } else {
            LOG_INF("Watchdog fed");
        }
    }

    return 0;
}

static int setup_watchdog(const struct device *wdt)
{
    int ret;

    const struct wdt_timeout_cfg cfg = {
        .window.min = 0,
        .window.max = WDT_TIMEOUT_MS,
        .flags = WDT_FLAG_RESET_SOC,
        .callback = NULL
    };

    ret = wdt_install_timeout(wdt, &cfg);
    if (ret < 0) {
        LOG_ERR("Installing watchdog timeout failed, error %d", ret);
        return ret;
    }

    int channel_id = ret;

    ret = wdt_setup(wdt, WDT_FLAG_RESET_SOC);
    if (ret < 0) {
        LOG_ERR("Setting up watchdog failed, error %d", ret);
        return ret;
    }

    return channel_id;
}
