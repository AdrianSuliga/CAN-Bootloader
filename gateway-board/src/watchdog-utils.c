#include <zephyr/kernel.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(WatchdogUtils, LOG_LEVEL_DBG);

static const struct device *wdt = DEVICE_DT_GET(DT_ALIAS(watchdog0));
static int channel_id = -1;

int setup_watchdog()
{
    int ret;

    if (!device_is_ready(wdt)) {
        LOG_ERR("Watchdog not ready!");
        return -1;
    }

    const struct wdt_timeout_cfg cfg = {
        .window.min = 0,
        .window.max = CONFIG_WATCHDOG_TIMEOUT,
        .flags = WDT_FLAG_RESET_SOC,
        .callback = NULL
    };

    ret = wdt_install_timeout(wdt, &cfg);
    if (ret < 0) {
        LOG_ERR("Installing watchdog timeout failed, error %d", ret);
        return ret;
    }

    channel_id = ret;

    return wdt_setup(wdt, WDT_FLAG_RESET_SOC);
}

int feed_watchdog()
{
    return wdt_feed(wdt, channel_id);
}
