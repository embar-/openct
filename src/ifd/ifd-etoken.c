/*
 * eToken driver
 *
 */

#include <stdlib.h>
#include <ifd/core.h>
#include <ifd/driver.h>
#include <ifd/device.h>
#include <ifd/logging.h>
#include <ifd/config.h>
#include <ifd/error.h>

#define ET_TIMEOUT	1000

static int	et_control(ifd_device_t *dev, int requesttype, int request,
			       int value, int index,
			       void *buf, size_t len,
			       long timeout);

/*
 * Initialize the device
 */
static int
et_open(ifd_reader_t *reader, const char *device_name)
{
	ifd_device_t *dev;

	reader->name = "Aladdin eToken PRO";
	reader->nslots = 1;
	if (!(dev = ifd_device_open(device_name)))
		return -1;
	if (ifd_device_type(dev) != IFD_DEVICE_TYPE_USB) {
		ifd_error("etoken: device %s is not a USB device",
				device_name);
		ifd_device_close(dev);
		return -1;
	}

	reader->device = dev;
	return 0;
}

/*
 * Power up the reader
 */
static int
et_activate(ifd_reader_t *reader)
{
	return 0;
}

static int
et_deactivate(ifd_reader_t *reader)
{
	return -1;
}

/*
 * Card status - always present
 */
static int
et_card_status(ifd_reader_t *reader, int slot, int *status)
{
	*status = IFD_CARD_PRESENT;
	return 0;
}

/*
 * Reset - nothing to be done?
 * We should do something to make it come back with all state zapped
 */
static int
et_card_reset(ifd_reader_t *reader, int slot, void *atr, size_t size)
{
	ifd_device_t *dev = reader->device;
	unsigned char	buffer[256];
	int		rc, n;

	/* Request the ATR */
	rc = et_control(dev, 0x40, 0x01, 0, 0, NULL, 0, ET_TIMEOUT);
	if (rc < 0)
		goto failed;

	/* Receive the ATR */
	rc = et_control(dev, 0xc0, 0x81, 0, 0, buffer, 0x23, ET_TIMEOUT);
	if (rc <= 0)
		goto failed;

	n = buffer[0];
	if (n + 1 > rc)
		goto failed;
	if (n > IFD_MAX_ATR_LEN)
		goto failed;

	if (n > size)
		n = size;
	memcpy(atr, buffer + 1, n);
	return n;

failed:	ifd_error("etoken: failed to activate token");
	return -1;
}

/*
 * Send/receive routines
 */
static int
et_send(ifd_reader_t *reader, unsigned int dad, const void *buffer, size_t len)
{
	return et_control(reader->device, 0x40, 0x06, 0, 0, buffer, len, -1);
}

static int
et_recv(ifd_reader_t *reader, unsigned int dad, void *buffer, size_t len, long timeout)
{
	return et_control(reader->device, 0xc0, 0x86, 0, 0, buffer, len, timeout);
}

/*
 * Send USB control message
 */
int
et_control(ifd_device_t *dev, int requesttype, int request,
	       int value, int index,
	       void *buf, size_t len,
	       long timeout)
{
	struct ifd_usb_cmsg cmsg;

	cmsg.guard = IFD_DEVICE_TYPE_USB;
	cmsg.requesttype = requesttype;
	cmsg.request = request;
	cmsg.value = value;
	cmsg.index = index;
	cmsg.data  = buf;
	cmsg.len = len;

	return ifd_device_control(dev, &cmsg, sizeof(cmsg));
}

/*
 * Driver operations
 */
static struct ifd_driver_ops	etoken_driver = {
	open:		et_open,
//	close:		et_close,
	activate:	et_activate,
	deactivate:	et_deactivate,
	card_status:	et_card_status,
	card_reset:	et_card_reset,
	send:		et_send,
	recv:		et_recv,
};
/*
 * Initialize this module
 */
void
ifd_init_module(void)
{
	ifd_driver_register("etoken", &etoken_driver);
}