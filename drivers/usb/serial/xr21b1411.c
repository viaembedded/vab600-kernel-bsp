/*
 * xr21b1411.c
 *
 * Copyright (c) 2012 Exar Corporation, Inc.
 *
 * ChangeLog:
 * 
 *            v0.4- Support for Kernel 3.0 and above (Ubuntu 11.10) 
 *		    (Removed all Kernel source compiler conditions 
 *     		    and now the base is Kernel 3.0)
 *            v0.1
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#define DRIVER_VERSION "v.0.4"
#define DRIVER_AUTHOR "Rob Duncan <rob.duncan@exar.com>"
/* 
 * post v0.1 modifications are by Ravi Reddy
 */
#define DRIVER_DESC "USB Driver for xr21b1411 USB serial port"

#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/errno.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/usb/serial.h>
#include <linux/serial.h>
#include <linux/slab.h>
#include <asm/unaligned.h>
#include <asm/uaccess.h>

#include <linux/usb/cdc.h>
#ifndef CDC_DATA_INTERFACE_TYPE
#define CDC_DATA_INTERFACE_TYPE 0x0a
#endif
#ifndef USB_RT_ACM
#define USB_RT_ACM      (USB_TYPE_CLASS | USB_RECIP_INTERFACE)
#define ACM_CTRL_DTR            0x01
#define ACM_CTRL_RTS            0x02
#define ACM_CTRL_DCD            0x01
#define ACM_CTRL_DSR            0x02
#define ACM_CTRL_BRK            0x04
#define ACM_CTRL_RI             0x08
#define ACM_CTRL_FRAMING        0x10
#define ACM_CTRL_PARITY         0x20
#define ACM_CTRL_OVERRUN        0x40
#endif

#include "linux/version.h"


#define N_IN_URB    4
#define N_OUT_URB   4
#define IN_BUFLEN   4096

static int debug;

#define DEBUG_CONFIG    (debug > 0)
#define DEBUG_BULK      (debug > 1)
#define DEBUG_DATA      (debug > 2)



/* -------------------------------------------------------------------------- */

#if defined(RHEL_RELEASE_CODE)
#if RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(5, 2)
#define true 1

static inline int usb_endpoint_dir_in(const struct usb_endpoint_descriptor *epd)
{
        return ((epd->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN);
}

static inline int usb_endpoint_dir_out(const struct usb_endpoint_descriptor *epd)
{
        return ((epd->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT);
}

static inline int usb_endpoint_xfer_bulk(const struct usb_endpoint_descriptor *epd)
{
        return ((epd->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
                USB_ENDPOINT_XFER_BULK);
}

static inline int usb_endpoint_is_bulk_in(const struct usb_endpoint_descriptor *epd)
{
        return (usb_endpoint_xfer_bulk(epd) && usb_endpoint_dir_in(epd));
}

static inline int usb_endpoint_is_bulk_out(const struct usb_endpoint_descriptor *epd)
{
        return (usb_endpoint_xfer_bulk(epd) && usb_endpoint_dir_out(epd));
}
#endif
#endif

/* -------------------------------------------------------------------------- */

#include "xr21b1411ioctl.h"
#include "xr21b1411.h"

/* -------------------------------------------------------------------------- */

static struct usb_device_id id_table [] = {
        { USB_DEVICE(0x04e2, 0x1411) },
        { }
};
MODULE_DEVICE_TABLE(usb, id_table);


static void xr21b1411_disconnect(struct usb_interface *interface);

static struct usb_driver xr21b1411_driver = {
        .name          = "xr21b1411",
        .probe         = usb_serial_probe,
        .disconnect    = xr21b1411_disconnect,
        .id_table      = id_table,
        .no_dynamic_id = 1,
};


/* -------------------------------------------------------------------------- */

struct xr21b1411_serial_private
{
        struct usb_interface *data_interface;
};


struct xr21b1411_port_private {
        spinlock_t                  lock; /* lock the structure */
        int                         outstanding_urbs; /* number of out urbs in flight */

        struct urb                 *in_urbs[N_IN_URB];
        char                       *in_buffer[N_IN_URB];

        int                         ctrlin;
        int                         ctrlout;
        int                         clocal;

        struct usb_cdc_line_coding  line_coding;

        int                         preciseflags; /* USB: wide mode, TTY: flags per character */
        int                         trans9; /* USB: wide mode, serial 9N1 */
        int                         match; /* address match mode */

        int                         bcd_device;
};


/* -------------------------------------------------------------------------- */

#define XR_SET_REG              0
#define XR_GETN_REG             1


/* -------------------------------------------------------------------------- */

static int acm_ctrl_msg(struct usb_serial_port *port,
                        int request, int value, void *buf, int len)
{
        struct usb_serial *serial = port->serial;
        int retval = usb_control_msg(serial->dev,
                                     usb_sndctrlpipe(serial->dev, 0),
                                     request,
                                     USB_RT_ACM,
                                     value,
                                     serial->interface->cur_altsetting->desc.bInterfaceNumber,
                                     buf,
                                     len,
                                     5000);
        if (DEBUG_CONFIG) dev_dbg(&port->dev, "acm_control_msg: rq: 0x%02x val: %#x len: %#x result: %d\n", request, value, len, retval);
        return retval < 0 ? retval : 0;
}


#define acm_set_control(port, control)                                  \
        acm_ctrl_msg(port, USB_CDC_REQ_SET_CONTROL_LINE_STATE, control, NULL, 0)
#define acm_set_line(port, line)                                        \
        acm_ctrl_msg(port, USB_CDC_REQ_SET_LINE_CODING, 0, line, sizeof *(line))
#define acm_send_break(port, ms)                                \
        acm_ctrl_msg(port, USB_CDC_REQ_SEND_BREAK, ms, NULL, 0)



static int xr21b1411_test_mode(struct usb_serial_port *port,
                           int selector)
{
        struct usb_serial *serial = port->serial;
        int retval = usb_control_msg(serial->dev, usb_sndctrlpipe(serial->dev, 0),
                                     USB_REQ_SET_FEATURE,
                                     USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE,
                                     USB_DEVICE_TEST_MODE,
                                     selector << 8,
                                     NULL, 0, 5000);
        if (DEBUG_CONFIG) dev_dbg(&port->dev, "xr21b1411_test_mode: selector=0x%02x\n", selector);
        return retval < 0 ? retval : 0;
}


static int xr21b1411_set_reg(struct usb_serial_port *port,
                         int address, int value)
{
        struct usb_serial *serial = port->serial;
        int result;


        if (DEBUG_CONFIG) dev_dbg(&port->dev, "%s 0x%02x = 0x%02x\n", __func__, address, value);

        result = usb_control_msg(serial->dev,                     /* usb device */
                                 usb_sndctrlpipe(serial->dev, 0), /* endpoint pipe */
                                 XR_SET_REG,                      /* request */
                                 USB_DIR_OUT | USB_TYPE_VENDOR,   /* request_type */
                                 value,                           /* request value */
                                 address,                         /* index */
                                 NULL,                            /* data */
                                 0,                               /* size */
                                 5000);                           /* timeout */

        return result;
}


static int xr21b1411_get_reg(struct usb_serial_port *port,
                         int address, u16 *value)
{
        struct usb_serial *serial = port->serial;
        int result;

        result = usb_control_msg(serial->dev,                     /* usb device */
                                 usb_rcvctrlpipe(serial->dev, 0), /* endpoint pipe */
                                 XR_GETN_REG,                     /* request */
                                 USB_DIR_IN | USB_TYPE_VENDOR,    /* request_type */
                                 0,                               /* request value */
                                 address,                         /* index */
                                 value,                           /* data */
                                 2,                               /* size */
                                 5000);                           /* timeout */

        return result;
}


static void xr21b1411_disable(struct usb_serial_port *port)
{
        xr21b1411_set_reg(port, XR21B1411_REG_UART + UART_ENABLE, 0);
}


static void xr21b1411_enable(struct usb_serial_port *port)
{
        xr21b1411_set_reg(port, XR21B1411_REG_UART + UART_ENABLE, UART_ENABLE_TX | UART_ENABLE_RX);
}


static void xr21b1411_loopback(struct usb_serial_port *port)
{
        dev_info(&port->dev, "Internal loopback\n");

        xr21b1411_disable(port);
        xr21b1411_set_reg(port, XR21B1411_REG_UART + UART_LOOPBACK, UART_LOOPBACK_TX_RX | UART_LOOPBACK_RTS_CTS);
        xr21b1411_enable(port);
}



static void xr21b1411_set_termios(struct tty_struct *tty_param,
                                struct usb_serial_port *port,
                                struct ktermios *old_termios)
{
        struct xr21b1411_port_private *portdata = usb_get_serial_port_data(port);

        unsigned int             cflag;
        unsigned int             flow, gpio_mode;
        struct tty_struct       *tty = port->port.tty;

        unsigned int data_bits, parity_type, char_format;
        int result;

        if (DEBUG_CONFIG) dev_dbg(&port->dev, "%s\n", __func__);

/*  mutex_lock(&config_mutex); */

        cflag = tty->termios->c_cflag;

        portdata->clocal = ((cflag & CLOCAL) != 0);

        xr21b1411_disable(port);

        if ((cflag & CSIZE) == CS7) {
                data_bits = 7;
        } else if ((cflag & CSIZE) == CS5) {
                /* Enabling 5-bit mode is really 9-bit mode! */
                data_bits = 9;
        } else {
                data_bits = 8;
        }
        portdata->trans9 = (data_bits == 9);

        if (cflag & PARENB) {
                if (cflag & PARODD) {
                        if (cflag & CMSPAR) {
                                parity_type = USB_CDC_MARK_PARITY;
                        } else {
                                parity_type = USB_CDC_ODD_PARITY;
                        }
                } else {
                        if (cflag & CMSPAR) {
                                parity_type = USB_CDC_SPACE_PARITY;
                        } else {
                                parity_type = USB_CDC_EVEN_PARITY;
                        }
                }
        } else {
                parity_type = USB_CDC_NO_PARITY;
        }

        if (cflag & CSTOPB) {
                char_format = USB_CDC_2_STOP_BITS;
        } else {
                char_format = USB_CDC_1_STOP_BITS;
        }

        if (portdata->match) {
                /* We're in address-match mode, so assume that the
                 * flow register and wide-mode are already set
                 * correctly; don't mess with it. */
        } else {
                if (cflag & CRTSCTS) {
                        flow      = UART_FLOW_MODE_HW;
                        gpio_mode = UART_GPIO_MODE_SEL_RTS_CTS;
                } else if (I_IXOFF(tty) || I_IXON(tty)) {
                        unsigned char   start_char = START_CHAR(tty);
                        unsigned char   stop_char  = STOP_CHAR(tty);
                        
                        flow      = UART_FLOW_MODE_SW;
                        gpio_mode = UART_GPIO_MODE_SEL_GPIO;
                        
                        xr21b1411_set_reg(port, XR21B1411_REG_UART + UART_XON_CHAR, start_char);
                        xr21b1411_set_reg(port, XR21B1411_REG_UART + UART_XOFF_CHAR, stop_char);
                } else {
                        flow      = UART_FLOW_MODE_NONE;
                        gpio_mode = UART_GPIO_MODE_SEL_GPIO;
                }
                
                xr21b1411_set_reg(port, XR21B1411_REG_UART + UART_FLOW, flow);
                xr21b1411_set_reg(port, XR21B1411_REG_UART + UART_GPIO_MODE, gpio_mode);

                if (portdata->trans9) {
                        /* Turn on wide mode if we're 9-bit transparent. */
                        xr21b1411_set_reg(port, XR21B1411_REG_EPM + EPM_WIDE, EPM_WIDE_EN);
                } else if (!portdata->preciseflags) {
                        /* Turn off wide mode unless we have precise flags. */
                        xr21b1411_set_reg(port, XR21B1411_REG_EPM + EPM_WIDE, 0);
                }
        }

        portdata->line_coding.dwDTERate   = cpu_to_le32(tty_get_baud_rate(tty));
        portdata->line_coding.bCharFormat = char_format;
        portdata->line_coding.bParityType = parity_type;
        portdata->line_coding.bDataBits   = data_bits;

        if (DEBUG_CONFIG) dev_dbg(&port->dev, "%s: line coding: rate=%d format=%d parity=%d bits=%d\n", __func__,
                           cpu_to_le32(tty_get_baud_rate(tty)),
                           char_format,
                           parity_type,
                           data_bits);
        result = acm_set_line(port, &portdata->line_coding);
        if (result < 0) {
                //dev_dbg(&port->dev, "%d != %ld\n", result, sizeof(portdata->line_coding));
                dev_dbg(&port->dev, "%s: cannot set line coding: rate=%d format=%d parity=%d bits=%d\n", __func__,
                        tty_get_baud_rate(tty),
                        portdata->line_coding.bCharFormat,
                        portdata->line_coding.bParityType,
                        portdata->line_coding.bDataBits);
        }

        xr21b1411_enable(port);

/*  mutex_unlock(&config_mutex); */
}


static void xr21b1411_break_ctl(struct tty_struct *tty, int break_state)
{
        struct usb_serial_port *port = tty->driver_data;

        if (DEBUG_CONFIG) dev_dbg(&port->dev, "BREAK %d\n", break_state);
        if (break_state)
                acm_send_break(port, 0x10);
        else
                acm_send_break(port, 0x000);
}


static int xr21b1411_tiocmget(struct tty_struct *tty)
{
        struct usb_serial_port *port = tty->driver_data;
        struct xr21b1411_port_private *portdata = usb_get_serial_port_data(port);

/*  if (!xr21b1411_READY(xr21b1411)) */
/*      return -EINVAL; */

        return (portdata->ctrlout & ACM_CTRL_DTR ? TIOCM_DTR : 0) |
                (portdata->ctrlout & ACM_CTRL_RTS ? TIOCM_RTS : 0) |
                (portdata->ctrlin  & ACM_CTRL_DSR ? TIOCM_DSR : 0) |
                (portdata->ctrlin  & ACM_CTRL_RI  ? TIOCM_RI  : 0) |
                (portdata->ctrlin  & ACM_CTRL_DCD ? TIOCM_CD  : 0) |
                TIOCM_CTS;
}


static int xr21b1411_tiocmset(struct tty_struct *tty, unsigned int set, unsigned int clear)
{
        struct usb_serial_port *port = tty->driver_data;
        struct xr21b1411_port_private *portdata = usb_get_serial_port_data(port);
        unsigned int newctrl;

/*  if (!xr21b1411_READY(xr21b1411)) */
/*      return -EINVAL; */

        newctrl = portdata->ctrlout;
        set = (set & TIOCM_DTR ? ACM_CTRL_DTR : 0) | (set & TIOCM_RTS ? ACM_CTRL_RTS : 0);
        clear = (clear & TIOCM_DTR ? ACM_CTRL_DTR : 0) | (clear & TIOCM_RTS ? ACM_CTRL_RTS : 0);

        newctrl = (newctrl & ~clear) | set;

        if (portdata->ctrlout == newctrl)
                return 0;
        return acm_set_control(port, portdata->ctrlout = newctrl);
}


static int xr21b1411_ioctl(struct tty_struct *tty, unsigned int cmd, unsigned long arg)
{
        struct usb_serial_port *port = tty->driver_data;
        struct xr21b1411_port_private *portdata = usb_get_serial_port_data(port);

        unsigned int             reg, val, match, preciseflags, unicast, broadcast, flow, selector;
        u16     *rdata;
        int                      result;
        struct serial_struct     ss;

        if (DEBUG_CONFIG) dev_dbg(&port->dev, "%s %08x\n", __func__, cmd);

/*  if (!xr21b1411_READY(xr21b1411)) */
/*      return -EINVAL; */

        switch (cmd) {
        case TIOCGSERIAL:
                if (!arg)
                        return -EFAULT;
                memset(&ss, 0, sizeof(ss));
                ss.baud_base = le32_to_cpu(portdata->line_coding.dwDTERate);
                if (copy_to_user((void __user *)arg, &ss, sizeof(ss)))
                        return -EFAULT;
                break;

        case TIOCSSERIAL:
                if (!arg)
                        return -EFAULT;
                if (copy_from_user(&ss, (void __user *)arg, sizeof(ss)))
                        return -EFAULT;
                portdata->line_coding.dwDTERate = cpu_to_le32(ss.baud_base);
                if (DEBUG_CONFIG) dev_dbg(&port->dev, "baud_base=%d\n", ss.baud_base);

/*      mutex_lock(&config_mutex); */
                xr21b1411_disable(port);
                if (DEBUG_CONFIG) dev_dbg(&port->dev, "%s: line coding: rate=%d format=%d parity=%d bits=%d\n", __func__,
                                   ss.baud_base,
                                   portdata->line_coding.bCharFormat,
                                   portdata->line_coding.bParityType,
                                   portdata->line_coding.bDataBits);
                result = acm_set_line(port, &portdata->line_coding);
                if (result < 0) {
                        //dev_dbg(&port->dev, "%d != %ld\n", result, sizeof(portdata->line_coding));
                        dev_dbg(&port->dev, "%s: cannot set line coding: rate=%d format=%d parity=%d bits=%d\n", __func__,
                                ss.baud_base,
                                portdata->line_coding.bCharFormat,
                                portdata->line_coding.bParityType,
                                portdata->line_coding.bDataBits);
                        return -EINVAL;
                }
                xr21b1411_enable(port);
/*      mutex_unlock(&config_mutex); */
                break;

        case XRUSBIOC_GET_REG:
                if (get_user(reg, (int __user *)(arg)))
                        return -EFAULT;

                rdata = kmalloc(sizeof(u16), GFP_KERNEL);
                if (rdata == NULL) {
                        dev_err(&port->dev, "%s - Cannot allocate USB buffer.\n", __func__);
                        return -ENOMEM;
                }

                result = xr21b1411_get_reg(port, reg, rdata);
                if (result != 2) {
                        dev_err(&port->dev, "Cannot get register (%d)\n", result);
                        kfree(rdata);
                        return -EFAULT;
                }

                if (put_user(le16_to_cpu(*rdata), (int __user *)(arg + 1 * sizeof(int)))) {
                        dev_err(&port->dev, "Cannot put user result\n");
                        kfree(rdata);
                        return -EFAULT;
                }

                kfree(rdata);
                break;

        case XRUSBIOC_SET_REG:
                if (get_user(reg, (int __user *)(arg)))
                        return -EFAULT;
                if (get_user(val, (int __user *)(arg + sizeof(int))))
                        return -EFAULT;

                result = xr21b1411_set_reg(port, reg, val);
                if (result < 0)
                        return -EFAULT;
                break;

        case XRUSBIOC_SET_ADDRESS_MATCH:
                match = arg;

                if (DEBUG_CONFIG) dev_dbg(&port->dev, "%s VIOC_SET_ADDRESS_MATCH %d\n", __func__, match);

                xr21b1411_disable(port);

                if (match == XRUSB_ADDRESS_MATCH_DISABLE) {
                        portdata->match = 0;
                        flow            = UART_FLOW_MODE_NONE;
                        if (DEBUG_CONFIG) dev_dbg(&port->dev, "address match: disable flow=%d\n",
                                                  flow);
                        xr21b1411_set_reg(port, XR21B1411_REG_UART + UART_FLOW, flow);
                } else {
                        portdata->match = 1;
                        flow            = UART_FLOW_MODE_ADDR_MATCH;
                        unicast         = (match >> XRUSB_ADDRESS_UNICAST_S) & 0xff;
                        broadcast       = (match >> XRUSB_ADDRESS_BROADCAST_S) & 0xff;

                        if (DEBUG_CONFIG) dev_dbg(&port->dev, "address match: enable flow=%d ucast=%d bcast=%u\n",
                                                  flow, unicast, broadcast);
                        xr21b1411_set_reg(port, XR21B1411_REG_UART + UART_FLOW, flow);
                        xr21b1411_set_reg(port, XR21B1411_REG_UART + UART_XON_CHAR, unicast);
                        xr21b1411_set_reg(port, XR21B1411_REG_UART + UART_XOFF_CHAR, broadcast);
                        xr21b1411_set_reg(port, XR21B1411_REG_EPM + EPM_WIDE, EPM_WIDE_EN);
                }

                xr21b1411_enable(port);
                break;

        case XRUSBIOC_SET_PRECISE_FLAGS:
                preciseflags = arg;

                if (DEBUG_CONFIG) dev_dbg(&port->dev, "%s VIOC_SET_PRECISE_FLAGS %d\n", __func__, preciseflags);

                xr21b1411_disable(port);

                if (preciseflags) {
                        portdata->preciseflags = 1;
                } else {
                        portdata->preciseflags = 0;
                }

                xr21b1411_set_reg(port, XR21B1411_REG_EPM + EPM_WIDE, portdata->preciseflags);

                xr21b1411_enable(port);
                break;

        case XRUSBIOC_TEST_MODE:
                selector = arg;
                if (DEBUG_CONFIG) dev_dbg(&port->dev, "%s VIOC_TEST_MODE 0x%02x\n", __func__, selector);
                xr21b1411_test_mode(port, selector);
                break;

        case XRUSBIOC_LOOPBACK:
                selector = arg;
                dev_dbg(&port->dev, "VIOC_LOOPBACK 0x%02x\n", selector);
                xr21b1411_loopback(port);
                break;

        default:
                return -ENOIOCTLCMD;
        }

        return 0;
}


/* -------------------------------------------------------------------------- */


static void xr21b1411_out_callback(struct urb *urb)
{
        struct usb_serial_port          *port     = urb->context;
        struct xr21b1411_port_private       *portdata = usb_get_serial_port_data(port);
        int                              status   = urb->status;
        unsigned long                    flags;

        if (DEBUG_BULK) dev_dbg(&port->dev, "%s - port %d\n", __func__, port->number);

        /* free up the transfer buffer, as usb_free_urb() does not do this */
        kfree(urb->transfer_buffer);

        if (status)
                if (DEBUG_CONFIG) dev_dbg(&port->dev, "%s - nonzero write bulk status received: %d\n",
                                   __func__, status);

        spin_lock_irqsave(&portdata->lock, flags);
        --portdata->outstanding_urbs;
        spin_unlock_irqrestore(&portdata->lock, flags);

        usb_serial_port_softint(port);
}


static int xr21b1411_write_room(struct tty_struct *tty)
{
        struct usb_serial_port *port = tty->driver_data;
        struct xr21b1411_port_private       *portdata = usb_get_serial_port_data(port);
        unsigned long                    flags;

        if (DEBUG_BULK) dev_dbg(&port->dev, "%s - port %d\n", __func__, port->number);

        /* try to give a good number back based on if we have any free urbs at
         * this point in time */
        spin_lock_irqsave(&portdata->lock, flags);
        if (portdata->outstanding_urbs > N_OUT_URB * 2 / 3) {
                spin_unlock_irqrestore(&portdata->lock, flags);
                if (DEBUG_BULK) dev_dbg(&port->dev, "%s - write limit hit\n", __func__);
                return 0;
        }
        spin_unlock_irqrestore(&portdata->lock, flags);

        return 2048;
}


static int xr21b1411_write(struct tty_struct *tty, struct usb_serial_port *port,
                         const unsigned char *buf, int count)
{
        struct xr21b1411_port_private       *portdata = usb_get_serial_port_data(port);
        struct usb_serial               *serial   = port->serial;
        int                              bufsize  = portdata->preciseflags ? count * 2 : count;
        unsigned long                    flags;
        unsigned char                   *buffer;
        struct urb                      *urb;
        int                              status;

        portdata = usb_get_serial_port_data(port);

        if (DEBUG_BULK) dev_dbg(&port->dev, "%s: write (%d chars)\n", __func__, count);

        spin_lock_irqsave(&portdata->lock, flags);
        if (portdata->outstanding_urbs > N_OUT_URB) {
                spin_unlock_irqrestore(&portdata->lock, flags);
                if (DEBUG_BULK) dev_dbg(&port->dev, "%s - write limit hit\n", __func__);
                return 0;
        }
        portdata->outstanding_urbs++;
        spin_unlock_irqrestore(&portdata->lock, flags);

        buffer = kmalloc(bufsize, GFP_ATOMIC);

        if (!buffer) {
                dev_err(&port->dev, "out of memory\n");
                count = -ENOMEM;
                goto error_no_buffer;
        }

        urb = usb_alloc_urb(0, GFP_ATOMIC);
        if (!urb) {
                dev_err(&port->dev, "no more free urbs\n");
                count = -ENOMEM;
                goto error_no_urb;
        }

        if (portdata->preciseflags) {
                unsigned char *b = buffer;
                int i;
                for (i = 0; i < count; ++i) {
                        *b++ = buf[i];
                        *b++ = 0;
                }
        }
        else
                memcpy(buffer, buf, count);

        if (DEBUG_DATA) usb_serial_debug_data(debug, &port->dev, __func__, bufsize, buffer);

        usb_fill_bulk_urb(urb, serial->dev,
                          usb_sndbulkpipe(serial->dev,
                                          port->bulk_out_endpointAddress),
                          buffer, bufsize, xr21b1411_out_callback, port);

        /* send it down the pipe */
        status = usb_submit_urb(urb, GFP_ATOMIC);
        if (status) {
                dev_err(&port->dev, "%s - usb_submit_urb(write bulk) failed "
                        "with status = %d\n", __func__, status);
                count = status;
                goto error;
        }

        /* we are done with this urb, so let the host driver
         * really free it when it is finished with it */
        usb_free_urb(urb);

        return count;
error:
        usb_free_urb(urb);
error_no_urb:
        kfree(buffer);
error_no_buffer:
        spin_lock_irqsave(&portdata->lock, flags);
        --portdata->outstanding_urbs;
        spin_unlock_irqrestore(&portdata->lock, flags);
        return count;
}



/* -------------------------------------------------------------------------- */

#define PRECISE_OVERRUN 0x08
#define PRECISE_FRAME   0x04
#define PRECISE_BREAK   0x02
#define PRECISE_PARITY  0x01



static void xr21b1411_in_callback(struct urb *urb)
{
        int                              endpoint        = usb_pipeendpoint(urb->pipe);
        struct usb_serial_port          *port            = urb->context;
        struct xr21b1411_port_private       *portdata        = usb_get_serial_port_data(port);
        struct tty_struct               *tty             = port->port.tty;
        int                              preciseflags    = portdata->preciseflags;
        char                            *transfer_buffer = urb->transfer_buffer;
        int                              length, room;
        int                              err;

        if (DEBUG_BULK) dev_dbg(&port->dev, "%s: %p\n", __func__, urb);

        if (urb->status) {
                if (DEBUG_CONFIG) dev_dbg(&port->dev, "%s: nonzero status: %d on endpoint %02x.\n",
                                   __func__, urb->status, endpoint);
                return;
        }

        length = urb->actual_length;
        if (length == 0) {
                if (DEBUG_CONFIG) dev_dbg(&port->dev, "%s: empty read urb received\n", __func__);
                err = usb_submit_urb(urb, GFP_ATOMIC);
                if (err)
                        dev_err(&port->dev, "resubmit read urb failed. (%d)\n", err);
                return;
        }

        if (DEBUG_DATA) usb_serial_debug_data(debug, &port->dev, __func__, length, transfer_buffer);

        length = (preciseflags) ? (length / 2) : length;

        room = tty_buffer_request_room(tty, length);
        if (room != length) {
                if (DEBUG_CONFIG) dev_dbg(&port->dev, "Not enough room in TTY buf, dropped %d chars.\n", length - room);
        }

        if (room) {
                if (preciseflags) {
                        char *dp = transfer_buffer;
                        int i, ch, ch_flags;

                        for (i = 0; i < room; ++i) {
                                char tty_flag;

                                ch       = *dp++;
                                ch_flags = *dp++;

                                if (!portdata->trans9 && (ch_flags & PRECISE_PARITY))
                                        tty_flag = TTY_PARITY;
                                else if (ch_flags & PRECISE_BREAK)
                                        tty_flag = TTY_BREAK;
                                else if (ch_flags & PRECISE_FRAME)
                                        tty_flag = TTY_FRAME;
                                else if (ch_flags & PRECISE_OVERRUN)
                                        tty_flag = TTY_OVERRUN;
                                else
                                        tty_flag = TTY_NORMAL;

                                tty_insert_flip_char(tty, ch, tty_flag);
                        }
                } else {
                        tty_insert_flip_string(tty, transfer_buffer, room);
                }

                tty_flip_buffer_push(tty);
        }

        err = usb_submit_urb(urb, GFP_ATOMIC);
        if (err)
                dev_err(&port->dev, "resubmit read urb failed. (%d)\n", err);
}


/* -------------------------------------------------------------------------- */

static void xr21b1411_int_callback(struct urb *urb)
{
        struct usb_serial_port          *port     = urb->context;
        struct xr21b1411_port_private     *portdata = usb_get_serial_port_data(port);
        struct tty_struct               *tty      = port->port.tty;

        struct usb_cdc_notification     *dr       = urb->transfer_buffer;
        unsigned char                   *data;
        int                              newctrl;
        int                              status;

        switch (urb->status) {
        case 0:
                /* success */
                break;
        case -ECONNRESET:
        case -ENOENT:
        case -ESHUTDOWN:
                /* this urb is terminated, clean up */
                if (DEBUG_CONFIG) dev_dbg(&port->dev, "urb shutting down with status: %d\n", urb->status);
                return;
        default:
                if (DEBUG_BULK) dev_dbg(&port->dev, "nonzero urb status received: %d\n", urb->status);
                goto exit;
        }

/*  if (!xr21b1411_READY(xr21b1411)) */
/*      goto exit; */

        data = (unsigned char *)(dr + 1);
        switch (dr->bNotificationType) {

        case USB_CDC_NOTIFY_NETWORK_CONNECTION:
                if (DEBUG_CONFIG) dev_dbg(&port->dev, "%s network\n", dr->wValue ? "connected to" : "disconnected from");
                break;

        case USB_CDC_NOTIFY_SERIAL_STATE:
                newctrl = le16_to_cpu(get_unaligned((__le16 *)data));

                if (!portdata->clocal && (portdata->ctrlin & ~newctrl & ACM_CTRL_DCD)) {
                        if (DEBUG_CONFIG) dev_dbg(&port->dev, "calling hangup\n");
                        tty_hangup(tty);
                }

                portdata->ctrlin = newctrl;

                if (DEBUG_CONFIG) dev_dbg(&port->dev, "input control lines: dcd%c dsr%c break%c ring%c framing%c parity%c overrun%c\n",
                                   portdata->ctrlin & ACM_CTRL_DCD ? '+' : '-',
                                   portdata->ctrlin & ACM_CTRL_DSR ? '+' : '-',
                                   portdata->ctrlin & ACM_CTRL_BRK ? '+' : '-',
                                   portdata->ctrlin & ACM_CTRL_RI  ? '+' : '-',
                                   portdata->ctrlin & ACM_CTRL_FRAMING ? '+' : '-',
                                   portdata->ctrlin & ACM_CTRL_PARITY ? '+' : '-',
                                   portdata->ctrlin & ACM_CTRL_OVERRUN ? '+' : '-');
                break;

        default:
                if (DEBUG_CONFIG) dev_dbg(&port->dev, "unknown notification %d received: index %d len %d data0 %d data1 %d\n",
                                   dr->bNotificationType, dr->wIndex,
                                   dr->wLength, data[0], data[1]);
                break;
        }
exit:
        if (DEBUG_BULK) dev_dbg(&port->dev, "Resubmitting interrupt IN urb %p\n", urb);
        status = usb_submit_urb(urb, GFP_ATOMIC);
        if (status)
                dev_err(&port->dev, "usb_submit_urb failed with result %d", status);
}


/* -------------------------------------------------------------------------- */

static int xr21b1411_open(struct tty_struct *tty_param, struct usb_serial_port *port)
{
        struct xr21b1411_port_private       *portdata;
        struct usb_serial               *serial = port->serial;
        int                              i;
        struct urb                      *urb;
        int                              result;

        portdata = usb_get_serial_port_data(port);

        if (DEBUG_CONFIG) dev_dbg(&port->dev, "%s\n", __func__);

        acm_set_control(port, portdata->ctrlout = ACM_CTRL_DTR | ACM_CTRL_RTS);

        /* Reset low level data toggle and start reading from endpoints */
        for (i = 0; i < N_IN_URB; i++) {
                if (DEBUG_CONFIG) dev_dbg(&port->dev, "%s urb %d\n", __func__, i);

                urb = portdata->in_urbs[i];
                if (!urb)
                        continue;
                if (urb->dev != serial->dev) {
                        if (DEBUG_CONFIG) dev_dbg(&port->dev, "%s: dev %p != %p\n", __func__,
                                           urb->dev, serial->dev);
                        continue;
                }

                /*
                 * make sure endpoint data toggle is synchronized with the
                 * device
                 */
/*      if (DEBUG_CONFIG) dev_dbg(&port->dev, "%s clearing halt on %x\n", __func__, urb->pipe); */
/*      usb_clear_halt(urb->dev, urb->pipe); */

                if (DEBUG_CONFIG) dev_dbg(&port->dev, "%s submitting urb %p\n", __func__, urb);
                result = usb_submit_urb(urb, GFP_KERNEL);
                if (result) {
                        dev_err(&port->dev, "submit urb %d failed (%d) %d\n",
                                i, result, urb->transfer_buffer_length);
                }
        }

        /* start up the interrupt endpoint if we have one */
        if (port->interrupt_in_urb) {
                result = usb_submit_urb(port->interrupt_in_urb, GFP_KERNEL);
                if (result)
                        dev_err(&port->dev, "submit irq_in urb failed %d\n",
                                result);
        }
        return 0;
}


static void xr21b1411_close(struct usb_serial_port *port)
{
        int                              i;
        struct usb_serial               *serial = port->serial;
        struct xr21b1411_port_private       *portdata;
        struct tty_struct               *tty    = port->port.tty;

        if (DEBUG_CONFIG) dev_dbg(&port->dev, "%s\n", __func__);
        portdata = usb_get_serial_port_data(port);

        acm_set_control(port, portdata->ctrlout = 0);

        if (serial->dev) {
                /* Stop reading/writing urbs */
                for (i = 0; i < N_IN_URB; i++)
                        usb_kill_urb(portdata->in_urbs[i]);
        }

        usb_kill_urb(port->interrupt_in_urb);

        tty = NULL; /* FIXME */
}


static int xr21b1411_attach(struct usb_serial *serial)
{
        struct xr21b1411_serial_private     *serial_priv       = usb_get_serial_data(serial);
        struct usb_interface            *interface         = serial_priv->data_interface;
        struct usb_host_interface       *iface_desc;
        struct usb_endpoint_descriptor  *endpoint;
        struct usb_endpoint_descriptor  *bulk_in_endpoint  = NULL;
        struct usb_endpoint_descriptor  *bulk_out_endpoint = NULL;

        struct usb_serial_port          *port;
        struct xr21b1411_port_private       *portdata;
        struct urb                      *urb;
        int                              i, j;

        /* Assume that there's exactly one serial port. */
        port = serial->port[0];

        if (DEBUG_CONFIG) dev_dbg(&port->dev, "%s\n", __func__);

        /* The usb_serial is now fully set up, but we want to make a
         * couple of modifications.  Namely, it was configured based
         * upon the control interface and not the data interface, so
         * it has no notion of the bulk in and out endpoints.  So we
         * essentially do some of the same allocations and
         * configurations that the usb-serial core would have done if
         * it had not made any faulty assumptions about the
         * endpoints. */

        iface_desc = interface->cur_altsetting;
        for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
                endpoint = &iface_desc->endpoint[i].desc;

                if (usb_endpoint_is_bulk_in(endpoint)) {
                        bulk_in_endpoint = endpoint;
                }

                if (usb_endpoint_is_bulk_out(endpoint)) {
                        bulk_out_endpoint = endpoint;
                }
        }

        if (!bulk_out_endpoint || !bulk_in_endpoint) {
                dev_dbg(&port->dev, "Missing endpoint!\n");
                return -EINVAL;
        }

        port->bulk_out_endpointAddress = bulk_out_endpoint->bEndpointAddress;
        port->bulk_in_endpointAddress = bulk_in_endpoint->bEndpointAddress;

        portdata = kzalloc(sizeof(*portdata), GFP_KERNEL);
        if (!portdata) {
                dev_dbg(&port->dev, "%s: kmalloc for xr21b1411_port_private (%d) failed!.\n",
                        __func__, i);
                return -ENOMEM;
        }
        spin_lock_init(&portdata->lock);
        for (j = 0; j < N_IN_URB; j++) {
                portdata->in_buffer[j] = kmalloc(IN_BUFLEN, GFP_KERNEL);
                if (!portdata->in_buffer[j]) {
                        for (--j; j >= 0; j--)
                                kfree(portdata->in_buffer[j]);
                        kfree(portdata);
                        return -ENOMEM;
                }
        }

        usb_set_serial_port_data(port, portdata);

        portdata->bcd_device = le16_to_cpu(serial->dev->descriptor.bcdDevice);

        /* initialize the in urbs */
        for (j = 0; j < N_IN_URB; ++j) {
                urb = usb_alloc_urb(0, GFP_KERNEL);
                if (urb == NULL) {
                  dev_dbg(&port->dev, "%s: alloc for in port failed.\n", __func__);
                  continue;
                }
                /* Fill URB using supplied data. */
                if (DEBUG_CONFIG) dev_dbg(&port->dev, "Filling URB %p, EP=%d buf=%p len=%d\n", urb, port->bulk_in_endpointAddress, portdata->in_buffer[j], IN_BUFLEN);
                usb_fill_bulk_urb(urb, serial->dev,
                                  usb_rcvbulkpipe(serial->dev,
                                                  port->bulk_in_endpointAddress),
                                  portdata->in_buffer[j], IN_BUFLEN,
                                  xr21b1411_in_callback, port);
                portdata->in_urbs[j] = urb;
        }

        xr21b1411_set_reg(port, XR21B1411_PRM_RAM + PRM_CUSTOM_DRIVER, PRM_CUSTOM_DRIVER_ACTIVE);

        return 0;
}


static void xr21b1411_serial_disconnect(struct usb_serial *serial)
{
        struct usb_serial_port          *port;
        struct xr21b1411_port_private       *portdata;
        int                              i, j;

        if (DEBUG_CONFIG) dev_dbg(&serial->dev->dev, "%s %p\n", __func__, serial);

        for (i = 0; i < serial->num_ports; ++i) {
                port = serial->port[i];
                if (!port)
                        continue;
                portdata = usb_get_serial_port_data(port);
                if (!portdata)
                        continue;

                xr21b1411_set_reg(port, XR21B1411_PRM_RAM + PRM_CUSTOM_DRIVER, 0);

                for (j = 0; j < N_IN_URB; j++) {
                        usb_kill_urb(portdata->in_urbs[j]);
                        usb_free_urb(portdata->in_urbs[j]);
                }
        }
}


static void xr21b1411_serial_release(struct usb_serial *serial)
{
        struct usb_serial_port          *port;
        struct xr21b1411_port_private       *portdata;
        int                              i, j;

        if (DEBUG_CONFIG) dev_dbg(&serial->dev->dev, "%s %p\n", __func__, serial);

        for (i = 0; i < serial->num_ports; ++i) {
                port = serial->port[i];
                if (!port)
                        continue;
                portdata = usb_get_serial_port_data(port);
                if (!portdata)
                        continue;

                for (j = 0; j < N_IN_URB; j++) {
                        kfree(portdata->in_buffer[j]);
                }
                kfree(portdata);
                usb_set_serial_port_data(port, NULL);
        }
}


/* -------------------------------------------------------------------------- */

static int xr21b1411_calc_num_ports(struct usb_serial *serial)
{
        return 1;
}


static int xr21b1411_probe(struct usb_serial *serial,
                         const struct usb_device_id *id)
{
        struct usb_interface                    *intf                     = serial->interface;
        unsigned char                           *buffer                   = intf->altsetting->extra;
        int                                      buflen                   = intf->altsetting->extralen;
        struct usb_device                       *usb_dev                  = interface_to_usbdev(intf);
        struct usb_cdc_union_desc               *union_header             = NULL;
        struct usb_cdc_country_functional_desc  *cfd                      = NULL;
        u8                                       ac_management_function   = 0;
        u8                                       call_management_function = 0;
        int                                      call_interface_num       = -1;
        int                                      data_interface_num;
        struct usb_interface                    *control_interface;
        struct usb_interface                    *data_interface;
        struct usb_endpoint_descriptor          *epctrl;
        struct usb_endpoint_descriptor          *epread;
        struct usb_endpoint_descriptor          *epwrite;
        struct xr21b1411_serial_private           *serial_priv;

        if (!buffer) {
                dev_err(&intf->dev, "Weird descriptor references\n");
                return -EINVAL;
        }

        if (!buflen) {
                if (intf->cur_altsetting->endpoint->extralen && intf->cur_altsetting->endpoint->extra) {
                        if (DEBUG_CONFIG) dev_dbg(&intf->dev, "Seeking extra descriptors on endpoint\n");
                        buflen = intf->cur_altsetting->endpoint->extralen;
                        buffer = intf->cur_altsetting->endpoint->extra;
                } else {
                        dev_err(&intf->dev, "Zero length descriptor references\n");
                        return -EINVAL;
                }
        }

        while (buflen > 0) {
                if (buffer[1] != USB_DT_CS_INTERFACE) {
                        dev_err(&intf->dev, "skipping garbage\n");
                        goto next_desc;
                }

                switch (buffer[2]) {
                case USB_CDC_UNION_TYPE: /* we've found it */
                        if (union_header) {
                                dev_err(&intf->dev, "More than one union descriptor, skipping ...\n");
                                goto next_desc;
                        }
                        union_header = (struct usb_cdc_union_desc *)buffer;
                        break;
                case USB_CDC_COUNTRY_TYPE: /* export through sysfs */
                        cfd = (struct usb_cdc_country_functional_desc *)buffer;
                        break;
                case USB_CDC_HEADER_TYPE: /* maybe check version */
                        break; /* for now we ignore it */
                case USB_CDC_ACM_TYPE:
                        ac_management_function = buffer[3];
                        break;
                case USB_CDC_CALL_MANAGEMENT_TYPE:
                        call_management_function = buffer[3];
                        call_interface_num = buffer[4];
                        if ((call_management_function & 3) != 3) {
/*              dev_err(&intf->dev, "This device cannot do calls on its own. It is no modem.\n"); */
                        }
                        break;
                default:
                        /* there are LOTS more CDC descriptors that
                         * could legitimately be found here.
                         */
                        if (DEBUG_CONFIG) dev_dbg(&intf->dev, "Ignoring descriptor: "
                                           "type %02x, length %d\n",
                                           buffer[2], buffer[0]);
                        break;
                }
        next_desc:
                buflen -= buffer[0];
                buffer += buffer[0];
        }

        if (!union_header) {
                if (call_interface_num > 0) {
                        if (DEBUG_CONFIG) dev_dbg(&intf->dev, "No union descriptor, using call management descriptor\n");
                        data_interface = usb_ifnum_to_if(usb_dev, (data_interface_num = call_interface_num));
                        control_interface = intf;
                } else {
                        if (DEBUG_CONFIG) dev_dbg(&intf->dev, "No union descriptor, giving up\n");
                        return -ENODEV;
                }
        } else {
                control_interface = usb_ifnum_to_if(usb_dev, union_header->bMasterInterface0);
                data_interface    = usb_ifnum_to_if(usb_dev, (data_interface_num = union_header->bSlaveInterface0));
                if (!control_interface || !data_interface) {
                        if (DEBUG_CONFIG) dev_dbg(&intf->dev, "no interfaces\n");
                        return -ENODEV;
                }
        }

        if (data_interface_num != call_interface_num)
                if (DEBUG_CONFIG) dev_dbg(&intf->dev, "Separate call control interface. That is not fully supported.\n");

        /* workaround for switched interfaces */
        if (data_interface->cur_altsetting->desc.bInterfaceClass != CDC_DATA_INTERFACE_TYPE) {
                if (control_interface->cur_altsetting->desc.bInterfaceClass == CDC_DATA_INTERFACE_TYPE) {
                        struct usb_interface *t;
/*          if (DEBUG_CONFIG) dev_dbg(&intf->dev, "Your device has switched interfaces.\n"); */

                        t = control_interface;
                        control_interface = data_interface;
                        data_interface = t;
                } else {
                        return -EINVAL;
                }
        }

        /* Accept probe requests only for the control interface */
        if (intf != control_interface) {
/*      if (DEBUG_CONFIG) dev_dbg(&intf->dev, "Skipping data interface %p\n", intf); */
                return -ENODEV;
        }
/*  if (DEBUG_CONFIG) dev_dbg(&intf->dev, "Grabbing control interface %p\n", intf); */

        if (usb_interface_claimed(data_interface)) { /* valid in this context */
                if (DEBUG_CONFIG) dev_dbg(&intf->dev, "The data interface isn't available\n");
                return -EBUSY;
        }

        if (data_interface->cur_altsetting->desc.bNumEndpoints < 2)
                return -EINVAL;

        epctrl  = &control_interface->cur_altsetting->endpoint[0].desc;
        epread  = &data_interface->cur_altsetting->endpoint[0].desc;
        epwrite = &data_interface->cur_altsetting->endpoint[1].desc;
        if (!usb_endpoint_dir_in(epread)) {
                struct usb_endpoint_descriptor *t;
                t   = epread;
                epread  = epwrite;
                epwrite = t;
        }

        /* The documentation suggests that we allocate private storage
         * with the attach() entry point, but we can't allow the data
         * interface to remain unclaimed until then; so we need
         * somewhere to save the claimed interface now. */
        if (!(serial_priv = kzalloc(sizeof(struct xr21b1411_serial_private), GFP_KERNEL))) {
                if (DEBUG_CONFIG) dev_dbg(&intf->dev, "out of memory\n");
                goto alloc_fail;
        }
        usb_set_serial_data(serial, serial_priv);

/*  if (DEBUG_CONFIG) dev_dbg(&intf->dev, "Claiming data interface %p\n", data_interface); */
        usb_driver_claim_interface(&xr21b1411_driver, data_interface, NULL);

        /* Don't set the data interface private data.  When we
         * disconnect we test this field against NULL to discover
         * whether we're dealing with the control or data
         * interface. */
        serial_priv->data_interface = data_interface;

        return 0;

alloc_fail:
        return -ENOMEM;
}


static void xr21b1411_disconnect(struct usb_interface *interface)
{
        struct usb_serial               *serial = usb_get_intfdata(interface);
        struct xr21b1411_serial_private   *serial_priv;

        if (DEBUG_CONFIG) dev_dbg(&interface->dev, "%s %p\n", __func__, interface);

        if (!serial) {
                /* NULL interface private data means that we're
                 * dealing with the data interface and not the control
                 * interface.  So we just bail and let the real clean
                 * up happen later when the control interface is
                 * disconnected. */
                return;
        }

        serial_priv = usb_get_serial_data(serial);

/*  if (DEBUG_CONFIG) dev_dbg(&interface->dev, "Releasing data interface %p.\n", serial_priv->data_interface); */
        usb_driver_release_interface(&xr21b1411_driver, serial_priv->data_interface);

        kfree(serial_priv);
        usb_set_serial_data(serial, NULL);

/*  if (DEBUG_CONFIG) dev_dbg(&interface->dev, "Disconnecting control interface\n"); */
        usb_serial_disconnect(interface);
}



static struct usb_serial_driver xr21b1411_device = {
        .driver = {
                .owner = THIS_MODULE,
                .name  = "xr21b1411",
        },
        .usb_driver        = &xr21b1411_driver,
        .description       = "xr21b1411 USB serial port",
        .id_table          = id_table,
        .calc_num_ports    = xr21b1411_calc_num_ports,
        .probe             = xr21b1411_probe,
        .open              = xr21b1411_open,
        .close             = xr21b1411_close,
        .write             = xr21b1411_write,
        .write_room        = xr21b1411_write_room,
        .ioctl             = xr21b1411_ioctl,
        .set_termios       = xr21b1411_set_termios,
        .break_ctl         = xr21b1411_break_ctl,
        .tiocmget          = xr21b1411_tiocmget,
        .tiocmset          = xr21b1411_tiocmset,
        .attach            = xr21b1411_attach,
        .disconnect        = xr21b1411_serial_disconnect,
        .release           = xr21b1411_serial_release,
        .read_int_callback = xr21b1411_int_callback,
};


/* Functions used by new usb-serial code. */
static int __init xr21b1411_init(void)
{
        int retval;
        retval = usb_serial_register(&xr21b1411_device);
        if (retval)
                goto failed_device_register;


        retval = usb_register(&xr21b1411_driver);
        if (retval)
                goto failed_driver_register;

        printk(KERN_INFO DRIVER_DESC ": " DRIVER_VERSION "\n");

        return 0;

failed_driver_register:
        usb_serial_deregister(&xr21b1411_device);
failed_device_register:
        return retval;
}


static void __exit xr21b1411_exit(void)
{
        usb_deregister(&xr21b1411_driver);
        usb_serial_deregister(&xr21b1411_device);
}

module_init(xr21b1411_init);
module_exit(xr21b1411_exit);


MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");

module_param(debug, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug, "Debug messages");

