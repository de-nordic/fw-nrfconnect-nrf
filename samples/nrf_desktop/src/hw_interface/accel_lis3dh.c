/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/* This is a temporary file utilizing SR3 Shield HW. */

#include <zephyr/types.h>

#include <soc.h>
#include <device.h>
#include <gpio.h>
#include <nrf_gpio.h>
#include <i2c.h>

#include "event_manager.h"
#include "button_event.h"
#include "power_event.h"

#define MODULE accel
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_ACCEL_LOG_LEVEL);


#define LIS3DH_ADDRESS	0x19

#define WHO_AM_I	0x0F
#define CTRL_REG1	0x20
#define CTRL_REG5	0x24
#define CTRL_REG6	0x25
#define REFERENCE_REG	0x26
#define INT1_CFG	0x30
#define INT1_SRC	0x31
#define INT1_THS	0x32

#define I_AM_LIS3DH	0x33


static struct device *i2c_dev;
static struct gpio_callback gpio_cb;

static bool active;


static int disable_irq(void)
{
	int err = i2c_reg_write_byte(i2c_dev, LIS3DH_ADDRESS, INT1_CFG, 0x00);

	if (err) {
		LOG_ERR("I2C read error (%d) from %s:%d",
				err, __func__, __LINE__);
	}
	return err;
}

static int enable_irq(void)
{
	int err = i2c_reg_write_byte(i2c_dev, LIS3DH_ADDRESS, INT1_CFG, 0x2A);

	if (err) {
		LOG_ERR("I2C read error (%d) from %s:%d",
				err, __func__, __LINE__);
	}
	return err;
}

static int clear_irq(void)
{
	u8_t val;

	int err = i2c_reg_read_byte(i2c_dev, LIS3DH_ADDRESS, INT1_SRC, &val);

	if (err) {
		LOG_ERR("I2C read error (%d) from %s:%d",
				err, __func__, __LINE__);
	}
	k_sleep(1);
	return err;
}

static int wakeup_mode_set(void)
{
	u8_t ctrl_regs[] = { 0x5F, 0x01, 0x40, 0x00, 0x00 };
	u8_t int1_ths[]  = { 0x08, 0x00 };

	int err = disable_irq();

	if (err) {
		return err;
	}

	/* TODO burst write seems not working
	 * err = i2c_burst_write(i2c_dev, LIS3DH_ADDRESS, CTRL_REG1 | BIT(7),
	 *		ctrl_regs, sizeof(ctrl_regs));
	 */
	for (size_t i = 0; i < sizeof(ctrl_regs); i++) {
		err |= i2c_reg_write_byte(i2c_dev, LIS3DH_ADDRESS,
				CTRL_REG1 + i, ctrl_regs[i]);
	}
	if (err) {
		return err;
	}
	/* err = i2c_burst_write(i2c_dev, LIS3DH_ADDRESS, INT1_THS | BIT(7),
	 *		int1_ths, sizeof(int1_ths));
	 */
	for (size_t i = 0; i < sizeof(int1_ths); i++) {
		err |= i2c_reg_write_byte(i2c_dev, LIS3DH_ADDRESS,
				INT1_THS + i, int1_ths[i]);
	}
	if (err) {
		return err;
	}

	/* Reset LIS3DH internal HP filter */
	u8_t val;

	err = i2c_reg_read_byte(i2c_dev, LIS3DH_ADDRESS, REFERENCE_REG, &val);
	if (err) {
		return err;
	}

	err = clear_irq();
	if (err) {
		return err;
	}

	return enable_irq();
}


static int idle_mode_set(void)
{
	u8_t ctrl_regs[] = { 0x58, 0x00, 0x00, 0x00, 0x00 };

	int err = disable_irq();

	if (err) {
		return err;
	}

	/* Update configuration */
	/* err = i2c_burst_write(i2c_dev, LIS3DH_ADDRESS, CTRL_REG1 | BIT(7),
	 *		ctrl_regs, sizeof(ctrl_regs));
	 */
	for (size_t i = 0; i < sizeof(ctrl_regs); i++) {
		err |= i2c_reg_write_byte(i2c_dev, LIS3DH_ADDRESS,
				CTRL_REG1 + i, ctrl_regs[i]);
	}
	if (err) {
		return err;
	}

	err = clear_irq();
	if (err) {
		return err;
	}

	/* Set Power Down mode. */
	return i2c_reg_write_byte(i2c_dev, LIS3DH_ADDRESS, CTRL_REG1, 0x08);
}

static bool verify_id(void)
{
	u8_t val;
	int err = i2c_reg_read_byte(i2c_dev, LIS3DH_ADDRESS, WHO_AM_I, &val);

	if (err) {
		LOG_ERR("I2C read error (%d) from %s:%d",
				err, __func__, __LINE__);
		return false;
	}

	if (val != I_AM_LIS3DH) {
		LOG_ERR("Wrong id (0x%x != 0x%x)", val, I_AM_LIS3DH);
		return false;
	}

	return true;
}

static int reset(void)
{
	/* Resetting takes time - time-out to complete depends on ODR.
	 * see AN3308 application note - page 11.
	 */
	/* Set fast ODR - 1.6 kHz. */
	int err =  i2c_reg_write_byte(i2c_dev, LIS3DH_ADDRESS, CTRL_REG1, 0x80);

	if (err) {
		LOG_ERR("I2C read error (%d) from %s:%d",
				err, __func__, __LINE__);
		return err;
	}

	/* Reboot */
	err =  i2c_reg_write_byte(i2c_dev, LIS3DH_ADDRESS, CTRL_REG5, 0x80);
	if (err) {
		LOG_ERR("I2C read error (%d) from %s:%d",
				err, __func__, __LINE__);
		return err;
	}

	k_sleep(1);

	return 0;
}

static void irq_callback(struct device *dev, struct gpio_callback *cb,
		u32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);
}

static void async_init_fn(struct k_work *work)
{
	i2c_dev = device_get_binding(DT_I2C_1_NAME);
	if (!i2c_dev) {
		LOG_ERR("Cannot get I2C device");
		return;
	}

	if (!verify_id()) {
		LOG_ERR("Device not recognized");
		return;
	}

	int err = reset();

	if (err) {
		LOG_ERR("Device cannot be initialized");
		return;
	}

	err = idle_mode_set();
	if (err) {
		LOG_ERR("Device cannot go to idle mode");
		return;
	}

	struct device *gpio_dev = device_get_binding(DT_GPIO_P0_DEV_NAME);

	if (!gpio_dev) {
		LOG_ERR("Cannot get GPIO device");
		return;
	}

	err = gpio_pin_configure(gpio_dev, 0x19, GPIO_DIR_IN | GPIO_INT |
			GPIO_INT_LEVEL | GPIO_INT_ACTIVE_HIGH |
			GPIO_INT_DEBOUNCE);
	if (err) {
		LOG_ERR("Cannot configure IRQ pin");
		return;
	}

	gpio_init_callback(&gpio_cb, irq_callback, BIT(0x19));

	err = gpio_add_callback(gpio_dev, &gpio_cb);
	if (err) {
		LOG_ERR("Cannot configure IRQ callback");
		return;
	}

	err = gpio_pin_enable_callback(gpio_dev, 0x19);
	if (err) {
		LOG_ERR("Cannot enable IRQ callback");
		return;
	}

	module_set_state(MODULE_STATE_READY);
}
K_WORK_DEFINE(accel_async_init, async_init_fn);

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			return false;
		}

		if (check_state(event, MODULE_ID(board), MODULE_STATE_READY)) {
			if (!active) {
				active = true;
				k_work_submit(&accel_async_init);
			}
			return false;
		}

		return false;
	}

	if (is_power_down_event(eh)) {
		if (active) {
			LOG_INF("Switched to wake up mode");
			active = false;
			if (wakeup_mode_set()) {
				LOG_ERR("Cannot switch to wake up mode");
			}

			module_set_state(MODULE_STATE_STANDBY);
		}

		return active;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE_EARLY(MODULE, power_down_event);
