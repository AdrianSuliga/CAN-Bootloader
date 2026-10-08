#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/can.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

LOG_MODULE_REGISTER(Main, LOG_LEVEL_INF);

#define CAN_FRAME_BOOTLOADER_CTRL_ID 0x400
#define BOOTLOADER_COMMAND_START     0x0

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);
static const struct device *can1      = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

static volatile bool bootloader_flag = 0;

static int gpio_setup();
static int can_setup();

static void rx_callback(const struct device *dev, struct can_frame *frame, void *user_data) 
{
    if (frame->id != CAN_FRAME_BOOTLOADER_CTRL_ID) {
        return;
    }

    if (frame->dlc != 1) {
        return;
    }

    if (frame->data[0] != BOOTLOADER_COMMAND_START) {
        return;
    }

    bootloader_flag = 1;
}

int main()
{
    int ret;

    ret = gpio_setup();
    if (ret < 0) {
        LOG_ERR("GPIO setup failed, error %d", ret);
        return ret;
    }

    ret = can_setup();
    if (ret < 0) {
        LOG_ERR("CAN setup failed, error %d", ret);
        return ret;
    }

    while (1) {
        if (bootloader_flag) {
            sys_reboot(SYS_REBOOT_COLD);
        }

        gpio_pin_toggle_dt(&led0);
        gpio_pin_toggle_dt(&led2);
        k_sleep(K_MSEC(500));
    }

    return 0;
}

static int gpio_setup()
{
    int ret;

    if (!device_is_ready(led0.port)) {
        LOG_ERR("led0 not ready");
        return -1;
    }

    if (!device_is_ready(led2.port)) {
        LOG_ERR("led2 not ready");
        return -1;
    }

    ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure led0, error %d", ret);
        return ret;
    }

    ret = gpio_pin_configure_dt(&led2, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure led2, error %d", ret);
        return ret;
    }

    return 0;
}

static int can_setup()
{
    if (!device_is_ready(can1)) {
        LOG_ERR("can1 not ready");
        return -1;
    }

    struct can_filter filter = {
        .id    = 0x0,
        .mask  = 0x0,
        .flags = 0x0
    };

    int ret = can_add_rx_filter(can1, &rx_callback, NULL, &filter);
    if (ret < 0) {
        LOG_ERR("Failed to set CAN filter, error %d", ret);
        return ret;
    }

    return can_start(can1);
}
