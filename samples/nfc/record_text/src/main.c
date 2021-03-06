/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <misc/reboot.h>

#include <nfc_t2t_lib.h>
#include <nfc/ndef/nfc_ndef_msg.h>
#include <nfc/ndef/nfc_text_rec.h>

#include <soc.h>
#include <device.h>
#include <gpio.h>

#define MAX_REC_COUNT	3

/* Change this if you have an LED connected to a custom port */
#ifndef LED0_GPIO_CONTROLLER
#define LED0_GPIO_CONTROLLER	LED0_GPIO_PORT
#endif

#define LED_PORT LED0_GPIO_CONTROLLER

/* Change this if you have an LED connected to a custom pin */
#define LED_PIN	LED0_GPIO_PIN

#define LED_ON  (u32_t)(0)
#define LED_OFF (u32_t)(1)

static struct device *dev;

/* Text message in English with its language code. */
/** @snippet [NFC text usage_1] */
static const u8_t en_payload[] = {
	'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '!'
};
static const u8_t en_code[] = {'e', 'n'};
/** @snippet [NFC text usage_1] */

/* Text message in Norwegian with its language code. */
static const u8_t no_payload[] = {
	'H', 'a', 'l', 'l', 'o', ' ', 'V', 'e', 'r', 'd', 'e', 'n', '!'
};
static const u8_t no_code[] = {'N', 'O'};

/* Text message in Polish with its language code. */
static const u8_t pl_payload[] = {
	'W', 'i', 't', 'a', 'j', ' ', 0xc5, 0x9a, 'w', 'i', 'e', 'c', 'i',
	'e', '!'
};
static const u8_t pl_code[] = {'P', 'L'};


static void nfc_callback(void *context,
			 enum nfc_t2t_event event,
			 const u8_t *data,
			 size_t data_length)
{
	ARG_UNUSED(context);
	ARG_UNUSED(data);
	ARG_UNUSED(data_length);

	switch (event) {
	case NFC_T2T_EVENT_FIELD_ON:
		gpio_pin_write(dev, LED_PIN, LED_ON);
		break;
	case NFC_T2T_EVENT_FIELD_OFF:
		gpio_pin_write(dev, LED_PIN, LED_OFF);
		break;
	default:
		break;
	}
}


/**
 * @brief Function for encoding the NDEF text message.
 */
static int welcome_msg_encode(u8_t *buffer, u32_t *len)
{
	/** @snippet [NFC text usage_2] */
	int err;

	/* Create NFC NDEF text record description in English */
	NFC_NDEF_TEXT_RECORD_DESC_DEF(nfc_en_text_rec,
				      UTF_8,
				      en_code,
				      sizeof(en_code),
				      en_payload,
				      sizeof(en_payload));
	/** @snippet [NFC text usage_2] */

	/* Create NFC NDEF text record description in Norwegian */
	NFC_NDEF_TEXT_RECORD_DESC_DEF(nfc_no_text_rec,
				      UTF_8,
				      no_code,
				      sizeof(no_code),
				      no_payload,
				      sizeof(no_payload));

	/* Create NFC NDEF text record description in Polish */
	NFC_NDEF_TEXT_RECORD_DESC_DEF(nfc_pl_text_rec,
				      UTF_8,
				      pl_code,
				      sizeof(pl_code),
				      pl_payload,
				      sizeof(pl_payload));

	/* Create NFC NDEF message description, capacity - MAX_REC_COUNT
	 * records
	 */
	/** @snippet [NFC text usage_3] */
	NFC_NDEF_MSG_DEF(nfc_text_msg, MAX_REC_COUNT);
	/** @snippet [NFC text usage_3] */

	/* Add text records to NDEF text message */
	/** @snippet [NFC text usage_4] */
	err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(nfc_text_msg),
				   &NFC_NDEF_TEXT_RECORD_DESC(nfc_en_text_rec));
	if (err < 0) {
		printk("Cannot add first record!\n");
		return err;
	}
	/** @snippet [NFC text usage_4] */
	err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(nfc_text_msg),
				   &NFC_NDEF_TEXT_RECORD_DESC(nfc_no_text_rec));
	if (err < 0) {
		printk("Cannot add second record!\n");
		return err;
	}

	err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(nfc_text_msg),
				   &NFC_NDEF_TEXT_RECORD_DESC(nfc_pl_text_rec));
	if (err < 0) {
		printk("Cannot add third record!\n");
		return err;
	}

	/** @snippet [NFC text usage_5] */
	err = nfc_ndef_msg_encode(&NFC_NDEF_MSG(nfc_text_msg),
				      buffer,
				      len);
	if (err < 0) {
		printk("Cannot encode message!\n");
	}

	return err;
	/** @snippet [NFC text usage_5] */
}


static int board_init(void)
{
	dev = device_get_binding(LED_PORT);
	if (!dev) {
		printk("LED device not found\n");
		return -ENXIO;
	}

	/* Set LED pin as output */
	int err = gpio_pin_configure(dev, LED_PIN, GPIO_DIR_OUT);

	if (err) {
		printk("Cannot configure led port!\n");
		return err;
	}
	/* Turn LED off */
	gpio_pin_write(dev, LED_PIN, LED_OFF);

	return 0;
}


int main(void)
{
	u8_t ndef_msg_buf[256]; /* Buffer used to hold an NFC NDEF message. */
	u32_t len = sizeof(ndef_msg_buf);

	printk("NFC configuration start\n");

	/* Configure LED-pins as outputs */
	if (board_init() < 0) {
		printk("Cannot initialize board!\n");
		goto fail;
	}

	/* Set up NFC */
	if (nfc_t2t_setup(nfc_callback, NULL) < 0) {
		printk("Cannot setup NFC T2T library!\n");
		goto fail;
	}


	/* Encode welcome message */
	if (welcome_msg_encode(ndef_msg_buf, &len) < 0) {
		printk("Cannot encode message!\n");
		goto fail;
	}


	/* Set created message as the NFC payload */
	if (nfc_t2t_payload_set(ndef_msg_buf, len) < 0) {
		printk("Cannot set payload!\n");
		goto fail;
	}


	/* Start sensing NFC field */
	if (nfc_t2t_emulation_start() < 0) {
		printk("Cannot start emulation!\n");
		goto fail;
	}
	printk("NFC configuration done\n");

	return 0;

fail:
#if CONFIG_REBOOT
	sys_reboot(SYS_REBOOT_COLD);
#endif /* CONFIG_REBOOT */

	return -EIO;
}
