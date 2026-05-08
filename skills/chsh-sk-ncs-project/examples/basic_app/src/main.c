/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <dk_buttons_and_leds.h>

LOG_MODULE_REGISTER(basic_app, LOG_LEVEL_INF);

/* Application state */
static bool app_running = true;
static uint32_t blink_interval_ms = CONFIG_BASIC_APP_LED_BLINK_INTERVAL_MS;

/* Button callback */
static void button_handler(uint32_t button_state, uint32_t has_changed)
{
	if (has_changed & button_state & DK_BTN1_MSK) {
		LOG_INF("Button 1 pressed");
		app_running = !app_running;
		LOG_INF("Application %s", app_running ? "started" : "paused");
	}

	if (has_changed & button_state & DK_BTN2_MSK) {
		LOG_INF("Button 2 pressed");
		/* Toggle between fast and slow blink */
		blink_interval_ms = (blink_interval_ms == 1000) ? 250 : 1000;
		LOG_INF("Blink interval set to %d ms", blink_interval_ms);
	}
}

#if CONFIG_BASIC_APP_ENABLE_SHELL
/* Shell command to start application */
static int cmd_start(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	app_running = true;
	shell_print(sh, "Application started");
	return 0;
}

/* Shell command to stop application */
static int cmd_stop(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	app_running = false;
	shell_print(sh, "Application stopped");
	return 0;
}

/* Shell command to set blink interval */
static int cmd_interval(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(sh, "Usage: interval <ms>");
		return -EINVAL;
	}

	blink_interval_ms = atoi(argv[1]);
	if (blink_interval_ms < 100 || blink_interval_ms > 10000) {
		shell_error(sh, "Interval must be between 100 and 10000 ms");
		blink_interval_ms = CONFIG_BASIC_APP_LED_BLINK_INTERVAL_MS;
		return -EINVAL;
	}

	shell_print(sh, "Blink interval set to %d ms", blink_interval_ms);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_app,
	SHELL_CMD(start, NULL, "Start the application", cmd_start),
	SHELL_CMD(stop, NULL, "Stop the application", cmd_stop),
	SHELL_CMD(interval, NULL, "Set blink interval <ms>", cmd_interval),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(app, &sub_app, "Application commands", NULL);
#endif /* CONFIG_BASIC_APP_ENABLE_SHELL */

/* Application entry point */
int main(void)
{
	int ret;
	bool led_state = false;

	LOG_INF("=========================================");
	LOG_INF("Basic NCS Application");
	LOG_INF("Version: 1.0.0");
	LOG_INF("Build time: %s %s", __DATE__, __TIME__);
	LOG_INF("=========================================");

	/* Initialize buttons */
	ret = dk_buttons_init(button_handler);
	if (ret) {
		LOG_ERR("Failed to initialize buttons: %d", ret);
		return ret;
	}

	/* Initialize LEDs */
	ret = dk_leds_init();
	if (ret) {
		LOG_ERR("Failed to initialize LEDs: %d", ret);
		return ret;
	}

	LOG_INF("Initialization complete");
	LOG_INF("Press Button 1 to pause/resume");
	LOG_INF("Press Button 2 to toggle blink speed");
#if CONFIG_BASIC_APP_ENABLE_SHELL
	LOG_INF("Shell commands: app start, app stop, app interval <ms>");
#endif

	/* Main application loop */
	while (1) {
		if (app_running) {
			/* Toggle LED */
			led_state = !led_state;
			if (led_state) {
				dk_set_led_on(DK_LED1);
			} else {
				dk_set_led_off(DK_LED1);
			}
		} else {
			/* When paused, turn off LED */
			dk_set_led_off(DK_LED1);
		}

		k_msleep(blink_interval_ms);
	}

	return 0;
}
