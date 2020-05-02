/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/hid.h>

static usbd_device *usbd_dev;
static volatile uint8_t mute_status = 0;

const struct usb_device_descriptor dev_descr = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0210,
	.bDeviceClass = 0,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = 0x0483,
	.idProduct = 0x5710,
	.bcdDevice = 0x0200,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

//0x85, 0x08, /* Report ID (0x08)                     */

static const uint8_t hid_report_descriptor[] = {
	0x05, 0x0b, /* USAGE_PAGE (Telephony)               */
	0x09, 0x05, /* USAGE (Headset)                      */
	0xa1, 0x01, /* COLLECTION (Application)             */
    0x15, 0x00, /* Logical minimum (0)                  */
    0x25, 0x01, /* Logical minimum (1)                  */
	0x09, 0x2f, /*   USAGE (Phone mute)                 */
	0x75, 0x01, /*     REPORT_SIZE (1)                  */
	0x95, 0x01, /*     REPORT_COUNT (1)                 */
	0x81, 0x06, /*     INPUT (Data,Var,Rel)             */
	0x75, 0x01, /*     REPORT_SIZE (1)                  */
	0x95, 0x07, /*     REPORT_COUNT (7)                 */
    0x81, 0x01, /*     INPUT (Cnst,Ary,Abs)             */
	0x75, 0x08, /*     REPORT_SIZE (8)                  */
	0x95, 0x03, /*     REPORT_COUNT (3)                 */
    0x81, 0x01, /*     INPUT (Cnst,Ary,Abs)             */
	0xc0        /* END_COLLECTION                       */
};

static const struct {
	struct usb_hid_descriptor hid_descriptor;
	struct {
		uint8_t bReportDescriptorType;
		uint16_t wDescriptorLength;
	} __attribute__((packed)) hid_report;
} __attribute__((packed)) hid_function = {
	.hid_descriptor = {
		.bLength = sizeof(hid_function),
		.bDescriptorType = USB_DT_HID,
		.bcdHID = 0x0111,
		.bCountryCode = 0,
		.bNumDescriptors = 1,
	},
	.hid_report = {
		.bReportDescriptorType = USB_DT_REPORT,
		.wDescriptorLength = sizeof(hid_report_descriptor),
	}
};

const struct usb_endpoint_descriptor hid_endpoint = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x81,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	.wMaxPacketSize = 4,
	.bInterval = 0x01,
};

const struct usb_interface_descriptor hid_iface = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_HID,
	.bInterfaceSubClass = 0, /* No subclass */
	.bInterfaceProtocol = 0, /* None */
	.iInterface = 0,

	.endpoint = &hid_endpoint,

	.extra = &hid_function,
	.extralen = sizeof(hid_function),
};

const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = &hid_iface,
}};

const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 1,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0xA0,
	.bMaxPower = 0x32,

	.interface = ifaces,
};

static const char *usb_strings[] = {
	"Black Sphere Technologies",
	"HID Demo",
	"DEMO",
};

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];

static enum usbd_request_return_codes hid_control_request(usbd_device *dev, struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
			void (**complete)(usbd_device *, struct usb_setup_data *))
{
	(void)complete;
	(void)dev;

	if((req->bmRequestType != 0x81) ||
	   (req->bRequest != USB_REQ_GET_DESCRIPTOR) ||
	   (req->wValue != 0x2200))
		return USBD_REQ_NOTSUPP;

	/* Handle the HID report descriptor. */
	*buf = (uint8_t *)hid_report_descriptor;
	*len = sizeof(hid_report_descriptor);

	return USBD_REQ_HANDLED;
}

static void hid_set_config(usbd_device *dev, uint16_t wValue)
{
	(void)wValue;
	(void)dev;

	usbd_ep_setup(dev, 0x81, USB_ENDPOINT_ATTR_INTERRUPT, 4, NULL);

	usbd_register_control_callback(
				dev,
				USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE,
				USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
				hid_control_request);
}

int main(void)
{
	//rcc_clock_setup_in_hsi_out_48mhz();
	rcc_clock_setup_pll(&rcc_hse_12mhz_3v3[RCC_CLOCK_3V3_84MHZ]);

	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOC);
  	rcc_periph_clock_enable(RCC_OTGFS);
    rcc_periph_clock_enable(RCC_SYSCFG);

    /* Set GPIO mode for LED */
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO2);
    gpio_set(GPIOC, GPIO2);

    /* Set GPIO for button and IRQ */
    gpio_mode_setup(GPIOC, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO4);
    nvic_enable_irq(NVIC_EXTI4_IRQ);
    exti_select_source(EXTI4, GPIOC);
    exti_set_trigger(EXTI4, EXTI_TRIGGER_FALLING);
    exti_enable_request(EXTI4);

    /* Set GPIO modes for USB D+ and D- */
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11 | GPIO12);
	gpio_set_af(GPIOA, GPIO_AF10, GPIO11 | GPIO12);

	usbd_dev = usbd_init(&otgfs_usb_driver, &dev_descr, &config, usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));
	usbd_register_set_config_callback(usbd_dev, hid_set_config);

	while (1)
		usbd_poll(usbd_dev);
}

void exti4_isr(void)
{
    exti_reset_request(EXTI4);
    uint8_t buf[4] = {0, 0, 0, 0};
    uint16_t res;
    //buf[0] = 0x08; /* Report ID */
    if( mute_status )
    {
        buf[0] = 0;
        mute_status = 0;
    } else {
        buf[0] = 1;
        mute_status = 1;
    }
	res = usbd_ep_write_packet(usbd_dev, 0x81, (const void *)buf, 4);
    return;
}

