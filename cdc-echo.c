/* cdc-echo.c - Simple USB CDC-ACM demo for libopencm3 on STM32F1 */

/* This file is in the public domain. */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/st_usbfs.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/usbstd.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define USB_VID 0x6666
#define USB_PID 0xCDC0

#define DATA_OUT_MAX_PACKET_LEN 64
#define DATA_IN_MAX_PACKET_LEN 64
#define ECHO_BUF_LEN 128  /* Must be at least DATA_OUT_MAX_PACKET_LEN. */

#define ARRAY_LEN(ARRAY) (sizeof(ARRAY) / sizeof(*(ARRAY)))

enum {
	DATA_END_OUT,
	DATA_END_IN,
	DATA_ENDS
};

enum {
	CDC_END_CONTROL,
	CDC_ENDS
};

enum {
	INT_CDC,
	INT_DATA,
	INTS
};

enum {
	USB_STRING_MANUFACTURER,
	USB_STRING_PRODUCT,
	USB_STRINGS
};

static const char *const usbStrings[USB_STRINGS] = {
	[USB_STRING_MANUFACTURER] = "test",
	[USB_STRING_PRODUCT] = "USB CDC-ACM echo",
};

static const struct {
	struct usb_cdc_header_descriptor headerDesc;
	struct usb_cdc_call_management_descriptor callManagementDesc;
	struct usb_cdc_acm_descriptor acmDesc;
	struct usb_cdc_union_descriptor unionDesc;
} __attribute__((packed)) usbCdcDesc = {
	.headerDesc = {
		.bFunctionLength = sizeof(usbCdcDesc.headerDesc),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_HEADER,
		.bcdCDC = 0x0120,
	},
	.callManagementDesc = {
		.bFunctionLength = sizeof(usbCdcDesc.callManagementDesc),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
		.bmCapabilities = 0,
		.bDataInterface = 0,
	},
	.acmDesc = {
		.bFunctionLength = sizeof(usbCdcDesc.acmDesc),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_ACM,
		.bmCapabilities = 0,
	},
	.unionDesc = {
		.bFunctionLength = sizeof(usbCdcDesc.unionDesc),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_UNION,
		.bControlInterface = INT_CDC,
		.bSubordinateInterface0 = INT_DATA,
	},
};
static const struct usb_endpoint_descriptor usbCdcEndDesc[CDC_ENDS] = {
	[CDC_END_CONTROL] = {
		.bLength = USB_DT_ENDPOINT_SIZE,
		.bDescriptorType = USB_DT_ENDPOINT,
		.bEndpointAddress = USB_ENDPOINT_ADDR_IN(3),
		.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
		.wMaxPacketSize = 1,
		.bInterval = 255,
	},
};
static const struct usb_interface_descriptor usbCdcIntDesc[] = {
	{
		.bLength = USB_DT_INTERFACE_SIZE,
		.bDescriptorType = USB_DT_INTERFACE,
		.bInterfaceNumber = INT_CDC,
		.bAlternateSetting = 0,
		.bNumEndpoints = ARRAY_LEN(usbCdcEndDesc),
		.bInterfaceClass = USB_CLASS_CDC,
		.bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
		.bInterfaceProtocol = USB_CDC_PROTOCOL_NONE,
		.iInterface = 0,
		.endpoint = usbCdcEndDesc,
		.extra = &usbCdcDesc,
		.extralen = sizeof(usbCdcDesc),
	},
};
static const struct usb_endpoint_descriptor usbDataEndDesc[DATA_ENDS] = {
	[DATA_END_OUT] = {
		.bLength = USB_DT_ENDPOINT_SIZE,
		.bDescriptorType = USB_DT_ENDPOINT,
		.bEndpointAddress = USB_ENDPOINT_ADDR_OUT(1),
		.bmAttributes = USB_ENDPOINT_ATTR_BULK,
		.wMaxPacketSize = DATA_OUT_MAX_PACKET_LEN,
		.bInterval = 0,
	},
	[DATA_END_IN] = {
		.bLength = USB_DT_ENDPOINT_SIZE,
		.bDescriptorType = USB_DT_ENDPOINT,
		.bEndpointAddress = USB_ENDPOINT_ADDR_IN(2),
		.bmAttributes = USB_ENDPOINT_ATTR_BULK,
		.wMaxPacketSize = DATA_IN_MAX_PACKET_LEN,
		.bInterval = 0,
	},
};
static const struct usb_interface_descriptor usbDataIntDesc[] = {
	{
		.bLength = USB_DT_INTERFACE_SIZE,
		.bDescriptorType = USB_DT_INTERFACE,
		.bInterfaceNumber = INT_DATA,
		.bAlternateSetting = 0,
		.bNumEndpoints = ARRAY_LEN(usbDataEndDesc),
		.bInterfaceClass = USB_CLASS_DATA,
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
		.iInterface = 0,
		.endpoint = usbDataEndDesc,
		.extra = NULL,
		.extralen = 0,
	},
};
static const struct usb_interface usbInt[INTS] = {
	[INT_CDC] = {
		.num_altsetting = ARRAY_LEN(usbCdcIntDesc),
		.altsetting = usbCdcIntDesc,
	},
	[INT_DATA] = {
		.num_altsetting = ARRAY_LEN(usbDataIntDesc),
		.altsetting = usbDataIntDesc,
	},
};
static const struct usb_config_descriptor usbConfigDesc[] = {
	{
		.bLength = USB_DT_CONFIGURATION_SIZE,
		.bDescriptorType = USB_DT_CONFIGURATION,
		.wTotalLength = 0,
		.bNumInterfaces = ARRAY_LEN(usbInt),
		.bConfigurationValue = 1,
		.iConfiguration = 0,
		.bmAttributes = USB_CONFIG_ATTR_DEFAULT,
		.bMaxPower = 50 / 2,
		.interface = usbInt,
	},
};
static const struct usb_device_descriptor usbDevDesc = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = 0,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = USB_VID,
	.idProduct = USB_PID,
	.bcdDevice = 0x0000,
	.iManufacturer = USB_STRING_MANUFACTURER + 1,
	.iProduct = USB_STRING_PRODUCT + 1,
	.iSerialNumber = 0,
	.bNumConfigurations = ARRAY_LEN(usbConfigDesc),
};

static bool usbdWritable = false;
static bool startIn = false;
static uint_least8_t echoBuf[ECHO_BUF_LEN];
static uint_fast16_t echoBufFill = 0;

static void usbDataOut (usbd_device *usbdDev, uint8_t endpointAddress) {
	uint_fast16_t packetLen = usbd_ep_read_packet(usbdDev, endpointAddress, echoBuf + echoBufFill, sizeof(echoBuf) - echoBufFill);
	if (packetLen) {
		if (!echoBufFill)
			startIn = true;
		echoBufFill += packetLen;

		if (echoBufFill > sizeof(echoBuf) - usbDataEndDesc[DATA_END_OUT].wMaxPacketSize)
			usbd_ep_nak_set(usbdDev, endpointAddress, 1);
	}
}

static void usbDataIn (usbd_device *usbdDev, uint8_t endpointAddress) {
	if (!echoBufFill)
		return;

	uint_fast16_t packetLen = echoBufFill;
	if (packetLen > usbDataEndDesc[DATA_END_IN].wMaxPacketSize)
		packetLen = usbDataEndDesc[DATA_END_IN].wMaxPacketSize;

	packetLen = usbd_ep_write_packet(usbdDev, endpointAddress, echoBuf, packetLen);

	if (packetLen) {
		echoBufFill -= packetLen;
		if (echoBufFill)
			memmove(echoBuf, echoBuf + packetLen, echoBufFill);

		if (echoBufFill <= sizeof(echoBuf) - usbDataEndDesc[DATA_END_OUT].wMaxPacketSize)
			usbd_ep_nak_set(usbdDev, usbDataEndDesc[DATA_END_OUT].bEndpointAddress, 0);
	}
}

static void setupEp (usbd_device *usbdDev, const struct usb_endpoint_descriptor endDesc[restrict static 1], usbd_endpoint_callback callback) {
	usbd_ep_setup(usbdDev, endDesc->bEndpointAddress, endDesc->bmAttributes, endDesc->wMaxPacketSize, callback);
}

static void usbSetConfig (usbd_device *usbdDev, uint16_t wValue) {
	(void)wValue;

	setupEp(usbdDev, &usbDataEndDesc[DATA_END_OUT], usbDataOut);
	setupEp(usbdDev, &usbDataEndDesc[DATA_END_IN], usbDataIn);
	setupEp(usbdDev, &usbCdcEndDesc[CDC_END_CONTROL], NULL);

	usbdWritable = true;
}

int main (void) {
	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	uint_least8_t usbControlBuffer[usbDevDesc.bMaxPacketSize0];
	usbd_device *usbdDev = usbd_init(&st_usbfs_v1_usb_driver, &usbDevDesc, usbConfigDesc, (const char **)usbStrings, ARRAY_LEN(usbStrings), usbControlBuffer, sizeof(usbControlBuffer));
	usbd_register_set_config_callback(usbdDev, usbSetConfig);

	for (;;) {
		usbd_poll(usbdDev);

		if (startIn && usbdWritable) {
			usbDataIn(usbdDev, usbDataEndDesc[DATA_END_IN].bEndpointAddress);
			startIn = false;
		}
	}
}
