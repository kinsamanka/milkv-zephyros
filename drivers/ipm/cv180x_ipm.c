/*
 * Copyright (c) 2024 GP Orcullo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sophgo_cv180x_ipm

#include <errno.h>
#include <zephyr/drivers/ipm.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cv180x_ipm, CONFIG_IPM_LOG_LEVEL);

#define NUM_SHARED_MB			2
#define NUM_DOORBELL			6
#define NUM_CHANS			(NUM_SHARED_MB + NUM_DOORBELL)

#define CV180X_MAX_ID_VAL			(NUM_CHANS - 1)

#define MAILBOX_ID_CPU1			1	/* c906B */
#define MAILBOX_ID_CPU2			2	/* c906L */

#define CV180X_SPINLOCK_TIMEOUT		10000
#define CV180X_SPINLOCK_WAIT_USEC		1
#define CV180X_SPINLOCK_IDX		0
#define CV180X_SPINLOCK_VAL 		(MAILBOX_ID_CPU2 << 4)

#define POLL_TIMEOUT_USEC		100000

#define CPU_ENABLE(i)			(0x0  + ((i) << 2))
#define CPU_INT_CLEAR(i)		(0x10 + ((i) << 4))
#define CPU_INT_MASK(i)			(0x14 + ((i) << 4))
#define CPU_INT_REQUEST(i)		(0x18 + ((i) << 4))
#define MBOX_SET 			0x60
#define HW_MUTEX			(0xC0 + (CV180X_SPINLOCK_IDX << 2))
#define MBOX_BUFFER(i,j)		(0x400 + ((i) << 3) + ((j) << 2))


struct cv180x_ipm_config {
	io_port_t base;
	void (*irq_config_func)(const struct device *dev);
};

struct cv180x_ipm_data {
	ipm_callback_t callback;
	void *callback_ctx;
};

static int cv180x_get_hw_mutex(io_port_t base, uint8_t mask)
{
	uint32_t count = k_us_to_cyc_ceil32(CV180X_SPINLOCK_TIMEOUT);
	uint32_t start = k_cycle_get_32();

	sys_write8(mask, base + HW_MUTEX);

	while (sys_read8(base + HW_MUTEX) != mask) {
		if (count < (k_cycle_get_32() - start))
			return -ETIMEDOUT;

		k_busy_wait(CV180X_SPINLOCK_WAIT_USEC);
		Z_SPIN_DELAY(10);

		sys_write8(mask, base + HW_MUTEX);
	}

	return 0;
}

static inline void cv180x_release_hw_mutex(io_port_t base, uint8_t mask)
{
	sys_write8(mask, base + HW_MUTEX);
}

static void cv180x_ipm_isr(const struct device *dev)
{
	struct cv180x_ipm_data *data = dev->data;
	const struct cv180x_ipm_config *config = dev->config;
	const int mbx = MAILBOX_ID_CPU2;
	uint8_t mask = CV180X_SPINLOCK_VAL;
	uint32_t buf[NUM_CHANS];
	uint32_t reg;
	int n;

	if (cv180x_get_hw_mutex(config->base, mask))
		LOG_ERR("Unable to secure hw mutex");

	reg = sys_read32(config->base + CPU_INT_REQUEST(mbx));

	if (!reg) {
		cv180x_release_hw_mutex(config->base, mask);
		LOG_ERR("Spurious mailbox interrupt");
		return;
	}

	for (n=0; n < NUM_CHANS; n++) {
		if (reg & BIT(n)) {
			buf[n] = -1;
			if (n < NUM_SHARED_MB)
				buf[n] = sys_read32(config->base + MBOX_BUFFER(mbx, n));

			LOG_DBG("Channel %d received 0x%08x", n, buf[n]);
		}
	}

	/* acknowledge request */
	sys_write32(reg, config->base + CPU_INT_CLEAR(mbx));

	/* clear cpu request */
	sys_clear_bits(config->base + CPU_ENABLE(mbx), reg);

	cv180x_release_hw_mutex(config->base, mask);

	for (n=0; n < NUM_CHANS; n++)
		if ((reg & BIT(n)) && data->callback)
			data->callback(dev, data->callback_ctx, n, &buf[n]);
}

static int cv180x_ipm_send(const struct device *dev, int wait,
			    uint32_t id, const void *data, int size)
{
	const struct cv180x_ipm_config *config = dev->config;
	const int mbx = MAILBOX_ID_CPU1;
	uint8_t mask = CV180X_SPINLOCK_VAL | (id + 1);

	if (id > CV180X_MAX_ID_VAL) {
		return -EINVAL;
	}

	if ((size < 0) || (size > sizeof(uint32_t))) {
		return -EMSGSIZE;
	}

	int ret = cv180x_get_hw_mutex(config->base, mask);
	if (ret) {
		LOG_ERR("Unable to secure hw mutex");
		return ret;
	}

	uint32_t val = data ? *(uint32_t *)data : -1;
	if (id < NUM_SHARED_MB)
		sys_write32(val, config->base + MBOX_BUFFER(mbx, id));

	/* clear & set cpu request */
	sys_write32(BIT(id), config->base + CPU_INT_CLEAR(mbx));
	sys_set_bit(config->base + CPU_ENABLE(mbx), id);
	/* trigger mailbox */
	sys_write32(BIT(id), config->base + MBOX_SET);

	cv180x_release_hw_mutex(config->base, mask);

	if (wait && !WAIT_FOR(!sys_test_bit(config->base + CPU_INT_REQUEST(mbx), id),
			      POLL_TIMEOUT_USEC,
			      k_yield()))
		return -EBUSY;

	return 0;
}

static int cv180x_ipm_max_data_size_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return sizeof(uint32_t);
}


static uint32_t cv180x_ipm_max_id_val_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return CV180X_MAX_ID_VAL;
}

static void cv180x_ipm_register_callback(const struct device *dev,
					  ipm_callback_t cb,
					  void *context)
{
	struct cv180x_ipm_data *driver_data = dev->data;

	driver_data->callback = cb;
	driver_data->callback_ctx = context;
}


static int cv180x_ipm_set_enabled(const struct device *dev, int enable)
{
	const struct cv180x_ipm_config *config = dev->config;

	if (enable) {
		sys_write32(0xff, config->base + CPU_INT_CLEAR(MAILBOX_ID_CPU2));
		sys_write32(0x00, config->base + CPU_INT_MASK(MAILBOX_ID_CPU2));
	} else {
		sys_write32(0xff, config->base + CPU_INT_MASK(MAILBOX_ID_CPU2));
	}
	return 0;
}

static int cv180x_ipm_init(const struct device *dev)
{
	const struct cv180x_ipm_config *config = dev->config;

	sys_write32(0xff, config->base + CPU_INT_CLEAR(MAILBOX_ID_CPU2));
	sys_write32(0xff, config->base + CPU_INT_MASK(MAILBOX_ID_CPU2));

	config->irq_config_func(dev);
	return 0;
}

static const struct ipm_driver_api cv180x_ipm_driver_api = {
	.send			= cv180x_ipm_send,
	.register_callback	= cv180x_ipm_register_callback,
	.max_data_size_get	= cv180x_ipm_max_data_size_get,
	.max_id_val_get		= cv180x_ipm_max_id_val_get,
	.set_enabled		= cv180x_ipm_set_enabled
};

static void cv180x_ipm_config_func_0(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    cv180x_ipm_isr,
		    DEVICE_DT_INST_GET(0),
		    0);

	irq_enable(DT_INST_IRQN(0));
}

static const struct cv180x_ipm_config cv180x_ipm_0_config = {
	.base			= DT_INST_REG_ADDR(0),
	.irq_config_func	= cv180x_ipm_config_func_0,
};

static struct cv180x_ipm_data cv180x_ipm_0_data;

DEVICE_DT_INST_DEFINE(0,
		      &cv180x_ipm_init,
		      NULL,
		      &cv180x_ipm_0_data, &cv180x_ipm_0_config,
		      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		      &cv180x_ipm_driver_api);
