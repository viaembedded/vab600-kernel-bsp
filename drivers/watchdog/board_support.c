/*
    Board Support Driver
    
        Kernel Part Stub for Board Support Service
        
    Features:
     * set / get parameters from serial flash via WMT api
     * set / unset watch dog

    Limitation:
     * Single process read / write only. No multi-process parallel access support
     
                                        jackwu, 20120723 updated
*/
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

/* for WMT registers */
#include <mach/hardware.h>
#include "board_support.h"

/* from parse_input.c */
extern int bs_parse_input(const char __user *buf, size_t count, loff_t *pos);
extern char* bs_parse_input_get_selected_name(void);

struct bs_dat_t {
	struct work_struct work;
	struct delayed_work io_work;
	/* software timer */
	struct timer_list timer;
	unsigned int timerexp;
};

static struct bs_dat_t gDat;

/* =============================================================================
 *         file interface
 * =============================================================================
 */

char *gEnvName;
char gEnvVal[256];
int gEnvLen;
int gEnvRead;

static int bs_fop_open(struct inode *inode, struct file *file)
{
	/* TODO: we may have a process lock here */
	gEnvRead = 0; /* reset read counter */
	return 0;
}

ssize_t bs_fop_read(struct file *filp, char __user *buf,
                    size_t count, loff_t *pos)
{
	if(gEnvRead == 0){
		/* possible first read, init ! */
		gEnvName = bs_parse_input_get_selected_name();
		if(gEnvName == NULL){
			ERR("BS: no selected env\n");
			return 0;
		}
		gEnvLen = 256;
		if(wmt_getsyspara(gEnvName, gEnvVal, &gEnvLen) != 0){
			ERR("BS: cannot get param\n");
			return -1;
		}
		if(count >= gEnvLen){
			/* easy! copy all */
			if(copy_to_user(buf, gEnvVal, gEnvLen) != 0){
				ERR("BS: cannot copy 1\n");
				return -1;
			}
			gEnvRead = gEnvLen;
			return gEnvLen;
		}else{
			/* oh, partial copy... */
			if(copy_to_user(buf, gEnvVal, count) != 0){
				ERR("BS: cannot copy 2\n");
				return -1;
			}
			gEnvRead = gEnvLen - count;
			return gEnvRead;
		}
	}else if(gEnvRead < gEnvLen){
		/* partial read but not complete yet */
		int left_len = gEnvLen - gEnvRead;
		if(count >= left_len){
			if(copy_to_user(buf, gEnvVal+gEnvRead, left_len) != 0){
				ERR("BS: cannot copy 3\n");
				return -1;
			}
			gEnvRead = gEnvLen;
			return left_len;
		}else{
			if(copy_to_user(buf, gEnvVal+gEnvRead, count) != 0){
				ERR("BS: cannot copy 4\n");
				return -1;
			}
			gEnvRead += count;
			return count;
		}
	}
	
	/* nothing to be done */
	return 0;
}

ssize_t bs_fop_write(struct file *filp, const char __user *buf,
                     size_t count, loff_t *pos)
{
	bs_parse_input(buf, count, pos);
	gEnvRead = 0; /* reset read counter for read */
	return count;
}


static int bs_fop_release(struct inode *inode, struct file *file)
{
	/* reserve for future use*/
	return 0;
}

static int bs_fop_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	unsigned int uint_val;
	
	//DBG("BS: ioctl cmd=%x g=%x s=%x e=%x\n", cmd, BS_GET_WDOG, BS_SET_WDOG, BS_SET_WDOG_EN);
	switch(cmd){
	case BS_GET_WDOG:
		if(put_user(bs_get_wdog(), (unsigned int __user *)arg)){
			ERR("BS: cannot put BS_GET_WDOG\n");
			return -EFAULT;
		}
		break;
	
	case BS_SET_WDOG:
		if(copy_from_user(&uint_val, (void __user *)arg, sizeof(unsigned int))){
			ERR("BS: cannot get BS_SET_WDOG\n");
			return -EFAULT;
		}
		bs_set_wdog(uint_val);
		break;

	case BS_SET_WDOG_EN:
		if(copy_from_user(&uint_val, (void __user *)arg, sizeof(unsigned int))){
			ERR("BS: cannot get BS_SET_WDOG_EN\n");
			return -EFAULT;
		}
		if(uint_val == 0){
			bs_wdog_disable();
		}else{
			bs_wdog_enable();
		}
		break;

	default:
		return -EFAULT;
	}
	return 0;
}

static unsigned int bs_fop_poll(struct file *filp, poll_table *wait)
{
	return 0;
}

static struct file_operations bs_fops = {
	.owner = THIS_MODULE,
	.open = bs_fop_open,
	.release = bs_fop_release,
	.unlocked_ioctl = bs_fop_ioctl,
	.read = bs_fop_read,
	.write = bs_fop_write,
	.poll = bs_fop_poll,
};

static struct miscdevice bs_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "bs",
	.fops = &bs_fops,
};

/* =============================================================================
 *         interrupt handler
 * =============================================================================
 */
static void bs_timeout_func(unsigned long _dat)
{
	/* schedule work */
	schedule_work(&gDat.work);
	/* set next timeout */
	mod_timer(&gDat.timer, jiffies + gDat.timerexp);
}

static void bs_work_func(struct work_struct *work)
{
	// reserved
}

/* =============================================================================
 *         Driver control
 * =============================================================================
 */

static int bs_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int bs_resume(struct platform_device *pdev)
{
	return 0;
}

static int bs_probe(struct platform_device *pdev)
{
	/* init work queue */
	INIT_WORK(&gDat.work, bs_work_func);
	/* timer 
	gDat.timerexp = HZ;
	setup_timer(&gDat.timer, bs_timeout_func, (unsigned long)&gDat);
	mod_timer(&gDat.timer, jiffies + gDat.timerexp);
	*/
	
	if(misc_register(&bs_miscdev)){
		ERR("BS: register dev failed");
	}
	
	return 0;
}

static int bs_remove(struct platform_device *pdev)
{
	return 0;
}

/* =============================================================================
 *         Driver register
 * =============================================================================
 */
static struct platform_device bs_dev = {
	.name	= "BoardSupport",
	.id	= 0,
};

static struct platform_driver bs_drv = {
	.probe		= bs_probe,
	.remove		= bs_remove,
	.suspend	= bs_suspend,
	.resume		= bs_resume,
	.driver = {
		.name = "BoardSupport",
	},
};

static int __init bs_init(void)
{
	if(platform_driver_register(&bs_drv)){
		return -1;
	}
	return platform_device_register(&bs_dev);
}

static void __exit bs_exit(void)
{
	INFO("BoardSupport driver exit\n");
	platform_device_unregister(&bs_dev);
	platform_driver_unregister(&bs_drv);
}

module_init(bs_init);
module_exit(bs_exit);

MODULE_LICENSE("GPL");

