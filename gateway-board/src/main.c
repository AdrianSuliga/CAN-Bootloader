#include "main.h"
#include "can-utils.h"

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

    ret = send_user_application_buffer(can_dev);
    if (ret < 0) {
        LOG_ERR("Failed to send new user application, error %d", ret);
        return 1;
    }

    LOG_INF("Going idle...");

    while (true) {
        gpio_pin_toggle_dt(&led);
        k_msleep(CAN_SEND_INTERVAL_MS);
    }

    return 0;
}
