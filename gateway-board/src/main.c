#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/can.h>

#define CAN_SEND_INTERVAL_MS 1000

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static const struct device *can_dev = DEVICE_DT_GET(DT_ALIAS(can0));

int main()
{
    if (!device_is_ready(led.port)) {
        printk("LED device not ready\n");
        return 1;
    }

    if (!device_is_ready(can_dev)) {
        printk("CAN device not ready\n");
        return 1;
    }

    int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        printk("Failed to configure LED as output, error %d\n", ret);
        return 1;
    }

    ret = can_start(can_dev);
    if (ret < 0) {
        printk("Failed to start CAN, error %d\n", ret);
        gpio_pin_set_dt(&led, 1);
        return 1;
    }

    while (1) {
        struct can_frame frame = {
            .id = 0x1,
            .flags = 0,
            .dlc = 2,
            .data = {0xAB, 0xCD}
        };

        ret = can_send(can_dev, &frame, K_FOREVER, NULL, NULL);
        if (ret < 0) {
            printk("Failed to send CAN frame, error %d\n", ret);
        }

        gpio_pin_toggle_dt(&led);
        k_msleep(CAN_SEND_INTERVAL_MS);
    }
    
    return 0;
}
