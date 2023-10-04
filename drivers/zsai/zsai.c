/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/zsai.h>
#include <zephyr/drivers/zsai_ioctl.h>
#include <stddef.h>
#include <sys/types.h>
#include <errno.h>

LOG_MODULE_REGISTER(zsai, CONFIG_ZSAI_LOG_LEVEL);

int zsai_ioctl(const struct device *dev, uint32_t id, const uintptr_t in, uintptr_t in_out)
{
	const struct zsai_driver_api *api = ZSAI_API_PTR(dev);

	if (!ZSAI_IOCTL_SYSCALL_BIT_IS_SET(api->sys_call_bitmap, id)) {
		/* Does not require system privileges */
		return api->user_ioctl(dev, id, in, in_out);
	}
	return zsai_sys_ioctl(dev, id, in, in_out);
}
