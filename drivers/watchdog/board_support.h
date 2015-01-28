#ifndef __board_support_H__
#define __board_support_H__

extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);
extern int wmt_setsyspara(char *varname, unsigned char *varval);

#define DBG(x...) printk(x)
#define ERR(x...) printk(KERN_ERR x)
#define INFO(x...) printk(x)

#define BSIO 0xA1
#define BS_GET_WDOG      _IOR(BSIO, 1, unsigned int)
#define BS_SET_WDOG      _IOW(BSIO, 2, unsigned int)
#define BS_SET_WDOG_EN   _IOW(BSIO, 3, int)

extern unsigned int bs_get_wdog(void);
extern int bs_set_wdog(unsigned int val);
extern void bs_wdog_enable(void);
extern void bs_wdog_disable(void);

#endif

