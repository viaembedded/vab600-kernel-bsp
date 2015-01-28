/*
   Watch Dog function for Board Support Driver
   
   OSM0_ADDR 0x0100 : OS Timer Match Register 
   OSCR_ADDR 0x0110 : OS Timer Count Register
   OSTS_ADDR 0x0114 : OS Timer Status Register
   OSTW_ADDR 0x0118 : OS Timer Watchdog Enable Register
   OSTI_ADDR 0x011C : OS Timer Interrupt Enable Register
   OSTC_ADDR 0x0120 : OS Timer Control Register
   OSTA_ADDR 0x0124 : OS Timer Access Status Register


   don't do this :
    DBG("BS: Reset count register\n");
    REG32_VAL(OSCR_ADDR) = 0x0; // set count register
    while(REG32_VAL(OSTA_ADDR) & BIT4 != 0){
        ;  // wait until written value valid
    }
   
   Graphics might use this timer, set this will freeze screen.

                                        jackwu, 20120723 updated
*/

/* for WMT registers */
#include <mach/hardware.h>
#include "board_support.h"

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/time.h>
#include <linux/poll.h>
#include <linux/ioctl.h>

#define WDOG_CLK 3000000  /* default might be 3 MHz in 8950 android BSP */

/* get OS Timer Count Value */
// Steven
// return: count
static unsigned int new_bs_get_wdog(void)
//unsigned int bs_get_wdog(void)
{
	REG32_VAL(OSTC_ADDR) |= BIT1; /* timer read request */
	while(REG32_VAL(OSTA_ADDR) & BIT5 != 0){
		;  /* wait until read value valid */
	}
	return REG32_VAL(OSCR_ADDR);
}

// Steven
// return: Sec
unsigned int bs_get_wdog(void)
{
  unsigned int nMatchCount;
  unsigned int nCurrentCount;
	// get match register
	nMatchCount = REG32_VAL(OSM0_ADDR);
	// get current count
	nCurrentCount = new_bs_get_wdog();
	// limit to 32bit then calculate seconds
	return ((nMatchCount - nCurrentCount) & 0xffffffff) / WDOG_CLK;
}

/* set value to OS Timer Match Register 0, which can trigger reset */
// Steven
//void bs_set_wdog(unsigned int val)
static void new_bs_set_wdog(unsigned int val)
{
	REG32_VAL(OSM0_ADDR) = val; /* set match register */
	while(REG32_VAL(OSTA_ADDR) & BIT0 != 0){
		;  /* wait until written value valid */
	}
}

// Steven: ref. bs_set_wdog_sec(int sec)
int bs_set_wdog(unsigned int nSec)
{
	unsigned int now;
  unsigned int nNewMatchCount;
	if(nSec > 0xffffffff / WDOG_CLK){
		return -1;
	}
	now = new_bs_get_wdog();
	nNewMatchCount = ((now + nSec * WDOG_CLK) & 0xffffffff);
	new_bs_set_wdog(nNewMatchCount);
	return 0;
}

/* set watch dog in second */
// Steven: the following code is wrong because bs_get_wdog(), bs_set_wdog() is not original.
void bs_set_wdog_sec(int sec)
{
	unsigned int now, sub, sub_op, val;
	/* we only count in 32-bit mode */
	if(sec > 0xffffffff / WDOG_CLK){
		sec = 0xffffffff / WDOG_CLK;
	}
	/* looped count, when counter > 0xffffffff, it continue count from 0 after overflow */
	now = bs_get_wdog();
	sub_op = sec * WDOG_CLK;
	sub = 0xffffffff - now;
	if(sub < sub_op){
		val = sub_op - sub;
	}else{
		val = now + sub_op;
	}
	bs_set_wdog(val);
}

/* enable OS Timer with Match Register 0 to emit reset event */
void bs_wdog_enable(void)
{
	REG32_VAL(OSTW_ADDR) |= BIT0;
}

/* disable OS Timer with Match Register 0 to emit reset event */
void bs_wdog_disable(void)
{
	printk("bs_wdog_disable\n");
	REG32_VAL(OSTW_ADDR) &= ~BIT0;
}


