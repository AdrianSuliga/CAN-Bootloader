#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define BLINK_INTERVAL_MS 1000

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

int main()
{
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

    while (1) {
        gpio_pin_toggle_dt(&led);
        k_msleep(BLINK_INTERVAL_MS);
    }
    
    return 0;
}
