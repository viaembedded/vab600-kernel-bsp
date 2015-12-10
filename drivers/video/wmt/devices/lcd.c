/*++ 
 * linux/drivers/video/wmt/lcd.c
 * WonderMedia video post processor (VPP) driver
 *
 * Copyright c 2012  WonderMedia  Technologies, Inc.
 *
 * This program is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 2 of the License, or 
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details. 
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * WonderMedia Technologies, Inc.
 * 4F, 533, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C
--*/

#define LCD_C
// #define DEBUG
/*----------------------- DEPENDENCE -----------------------------------------*/
#include "../lcd.h"
#include "../vout.h"

/*----------------------- PRIVATE MACRO --------------------------------------*/
/* #define  LCD_XXXX  xxxx    *//*Example*/

/*----------------------- PRIVATE CONSTANTS ----------------------------------*/
/* #define LCD_XXXX    1     *//*Example*/

/*----------------------- PRIVATE TYPE  --------------------------------------*/
/* typedef  xxxx lcd_xxx_t; *//*Example*/
typedef struct {
	lcd_parm_t* (*get_parm)(int arg);
} lcd_device_t;

// Currrent Available transmitter
lcd_transmitter_t *p_lcd_transmitter = 0;		

/*----------EXPORTED PRIVATE VARIABLES are defined in lcd.h  -------------*/
/*----------------------- INTERNAL PRIVATE VARIABLES - -----------------------*/
/* int  lcd_xxx;        *//*Example*/
lcd_device_t lcd_device_array[LCD_PANEL_MAX];
int lcd_panel_on = 0;
int lcd_pwm_enable;
lcd_panel_t lcd_panel_id = LCD_PANEL_MAX;

unsigned int lcd_panel_cap;
int lcd_panel_bpp;
int lcd_panel_msb;
int lcd_panel_dual;
int lcd_panel_hpixel;
int lcd_panel_vpixel;
int lcd_panel_fps;

extern vout_dev_t lcd_vout_dev_ops;

/*--------------------- INTERNAL PRIVATE FUNCTIONS ---------------------------*/
/* void lcd_xxx(void); *//*Example*/
#ifdef CONFIG_PWM_WMT
extern void pwm_set_enable(int no,int enable);
extern void pwm_set_freq(int no,unsigned int freq);
extern void pwm_set_level(int no,unsigned int level);

extern void pwm_set_scalar(int no,unsigned int scalar);
extern void pwm_set_period(int no,unsigned int period);
extern void pwm_set_duty(int no,unsigned int duty) ;
#endif

lcd_panel_t lcd_lvds_id = LCD_PANEL_MAX;

/*----------------------- Function Body --------------------------------------*/
/*----------------------- Backlight --------------------------------------*/
void lcd_set_lvds_id(int id)
{
	lcd_lvds_id = id;
}

int lcd_get_lvds_id(void)
{
	return lcd_lvds_id;
}
void lcd_set_parm(int ops, int bpp, int hpixel, int vpixel, int fps)
{

	if(ops&0x4)
		lcd_panel_id = LCD_OEM;
	else
		lcd_panel_id = (ops&0x1)?1:0;

	lcd_panel_msb = (ops & 0x8)?1:0;
	lcd_panel_dual = (ops & 0x2)?1:0;
	lcd_panel_cap = (ops & 0xF0) >> 4;	
	
	lcd_panel_bpp = bpp;
	lcd_panel_hpixel = hpixel;
	lcd_panel_vpixel = vpixel;
	lcd_panel_fps = fps;
}
vout_dev_t *lcd_get_dev(void)
{
	if( lcd_panel_id >= LCD_PANEL_MAX )
		return 0;

	return &lcd_vout_dev_ops;
}

void lcd_suspend(void){
	if (lcd_panel_on && p_lcd_transmitter && p_lcd_transmitter->suspend)
		p_lcd_transmitter->suspend();
}
void lcd_resume(void){
	if (lcd_panel_on && p_lcd_transmitter && p_lcd_transmitter->resume)
		p_lcd_transmitter->resume();
}

#ifdef __KERNEL__
/*----------------------- LCD --------------------------------------*/
static int __init lcd_arg_panel_id
(
	char *str			/*!<; // argument string */
)
{
	sscanf(str,"%d",(int *) &lcd_panel_id);
	if( lcd_panel_id >= LCD_PANEL_MAX ){
		lcd_panel_id = LCD_PANEL_MAX;
	}
	DBGMSG(KERN_INFO "set lcd panel id = %d\n",lcd_panel_id);	
  	return 1;
} /* End of lcd_arg_panel_id */

__setup("lcdid=", lcd_arg_panel_id);
#endif
int lcd_panel_register
(
	int no,						/*!<; //[IN] device number */
	void (*get_parm)(int mode)	/*!<; //[IN] get info function pointer */
)
{
	lcd_device_t *p;

	if( no >= LCD_PANEL_MAX ){
		DBGMSG(KERN_ERR "*E* lcd device no max is %d !\n",LCD_PANEL_MAX);
		return -1;
	}

	p = &lcd_device_array[no];
	if( p->get_parm ){
		DBGMSG(KERN_ERR "*E* lcd device %d exist !\n",no);
		return -1;
	}
	p->get_parm = (void *) get_parm;
//	printk("lcd_device_register %d 0x%x\n",no,p->get_parm);
	return 0;
} /* End of lcd_device_register */

lcd_parm_t *lcd_get_parm(lcd_panel_t id,unsigned int arg)
{
	lcd_device_t *p;

	p = &lcd_device_array[id];
	if( p && p->get_parm ){
		return p->get_parm(arg);
	}
	return 0;
}

/*----------------------- CH7305 --------------------------------------------------*/
/**************************************************
 * Add for CH7305
 * JerryWang(VIA Embedded)
 * 2013/03/20
 *****************************************************
 */
int lcd_transmitter_register(lcd_transmitter_t *ops)
{
	lcd_transmitter_t *list;
	
	if(ops && ops->init && ops->init())
		return -1;

	ops->next = 0;
	
	if(  p_lcd_transmitter == 0 )
		p_lcd_transmitter = ops;		
	else {
		list =  p_lcd_transmitter;
		while( list->next != 0 ){
			list = list->next;
		}
		list->next = ops;		
	}
//	MSG("[LCD] LCD Transmitter: %s\n", ops->name);

		return 0;
}

int get_lcd_setting(void)
{
        if ( !(p_lcd ||lcd_panel_id == LCD_OEM ))
                return -1;

        int x,y;
        x = lcd_panel_hpixel;//p_lcd->timing.hpixel;
        y = lcd_panel_vpixel;//p_lcd->timing.vpixel;

        if ((x == 800 && y == 480) ||
                (x == 800 && y == 600) ||
                (x == 1024 && y == 600) ||
                (x == 1024 && y == 768) ||
                (x == 1366 && y == 768)) {
                lcd_setting.panel_mode &= ~DUAL_CHANNEL;
        }
        else if((x == 1280 && y == 1024) ||
                (x == 1440 && y == 1050) ||
                (x == 1680 && y == 1050) ||
                (x == 1600 && y == 1200)) {
                lcd_setting.panel_mode |= DUAL_CHANNEL;
        }else if(lcd_panel_dual)  {
        	   lcd_setting.panel_mode |= DUAL_CHANNEL;
        }else
        	   lcd_setting.panel_mode &= ~DUAL_CHANNEL;
			
        	

        if ( lcd_panel_msb == 1 )
                lcd_setting.panel_mode |= LDI;
        else
                lcd_setting.panel_mode &= ~LDI;

	 if(lcd_panel_bpp == 24)
	 	lcd_setting.panel_mode |= OUTPUT_24;
	 else
	 	lcd_setting.panel_mode &= ~OUTPUT_24;

        lcd_setting.panel_mode &= ~EN_DITHERING;
        lcd_setting.deskew_xcmd = 2;

        return 0;
}

int transmitter_enable(void)
{
	lcd_transmitter_t *list;	
	if(  p_lcd_transmitter == 0 )
		return -1;
	else{
		list =  p_lcd_transmitter;
		do{
			if ( list->init() != 0 )
				//MSG("[LCD] Err: Transmitter %s registerred but failed to detect\n", list->name);
				list = list->next;				
			else{
				list->next = 0;
				p_lcd_transmitter = list;
				MSG("[LCD] LCD transmitter %s detected\n", list->name);
				//p_lcd_transmitter->power_off();
				return 0;
			}
		}while(list != 0);
	}
	
	p_lcd_transmitter = 0;

	return -1;
}


/*----------------------- vout device plugin --------------------------------------*/
void lcd_set_power_down(int enable)
{
	if (p_lcd_transmitter){
		if ( enable )
			p_lcd_transmitter->power_off();
		else
			p_lcd_transmitter->power_on();
	}

}

int lcd_set_mode(unsigned int *option)
{
	vout_t *vo;
	
	DBGMSG("option %d,%d\n",option[0],option[1]);

	vo = lcd_vout_dev_ops.vout;
	if( option ){
		
		// Set DVO
		govrh_set_dvo_clock_delay(vo->govr,(lcd_panel_cap & LCD_CAP_CLK_HI)? 0:1, 0);
		
		// Fix LVDS with VT1636 abnormal issue	
		if ((p_lcd_transmitter && p_lcd_transmitter->name == "vt1636") && (lcd_setting.panel_mode &DUAL_CHANNEL))
				govrh_set_dvo_clock_delay(vo->govr,(lcd_panel_cap & LCD_CAP_CLK_HI)? 0:1, 0x20);			
	
		govrh_set_dvo_sync_polar(vo->govr,(lcd_panel_cap & LCD_CAP_HSYNC_HI)? 0:1,(lcd_panel_cap & LCD_CAP_VSYNC_HI)? 0:1);
		switch( lcd_panel_bpp ){
			case 15:
				govrh_IGS_set_mode(vo->govr,0,1,1);
				break;
			case 16:
				govrh_IGS_set_mode(vo->govr,0,3,1);
				break;
			case 18:
				govrh_IGS_set_mode(vo->govr,0,2,1);
				break;
			case 24:
				govrh_IGS_set_mode(vo->govr,0,0,(lcd_setting.panel_mode & LDI)? 1:0);
				break;
			default:
				break;
		}

		// Set LCD transmitter
		if( p_lcd_transmitter ){
			p_lcd_transmitter->power_off();
			p_lcd_transmitter->set_mode(lcd_setting);
			p_lcd_transmitter->power_on();
		}

	}
	else {

		if (p_lcd_transmitter && p_lcd_transmitter->power_off)
			p_lcd_transmitter->power_off(); 

		p_lcd_transmitter = 0;
	}
	return 0;
}

int lcd_check_plugin(int hotplug)
{
	return (p_lcd ||lcd_panel_id == LCD_OEM )? 1:0;
}

int lcd_get_edid(char *buf)
{
	return 1;
}
	
int lcd_config(vout_info_t *info)
{
	info->resx = lcd_panel_hpixel;//p_lcd->timing.hpixel;
	info->resy = lcd_panel_vpixel;//p_lcd->timing.vpixel;
//	info->p_timing = &(p_lcd->timing);
	info->fps = lcd_panel_fps;//p_lcd->fps;
	return 0;
}

int lcd_init(vout_t *vo)
{
	if( lcd_panel_id >= LCD_PANEL_MAX ){
		return -1;
	}

	switch(lcd_panel_id){
		case 0:
			p_lcd = lcd_get_lvds_parm(lcd_panel_hpixel,lcd_panel_vpixel,lcd_panel_fps);
			break;
		case 1:
			p_lcd = lcd_get_ttl_parm(lcd_panel_hpixel,lcd_panel_vpixel,lcd_panel_fps);
			break;
		default:
			;
	}		

	if( p_lcd == 0 && lcd_panel_id != LCD_OEM) {
		printk("[Err]Fail to get panel timing. Please give timing by [wmt.display.tmr] in uboot.\n");
		return -1;
	}

	if (p_lcd){
		printk("Timing:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d\n",p_lcd->timing.pixel_clock,
			p_lcd->timing.option,p_lcd->timing.hsync,p_lcd->timing.hbp,
			p_lcd->timing.hpixel, p_lcd->timing.hfp,p_lcd->timing.vsync,
			p_lcd->timing.vbp,p_lcd->timing.vpixel,p_lcd->timing.vfp);

		// set default parameters
		vo->resx = p_lcd->timing.hpixel;
		vo->resy = p_lcd->timing.vpixel;
		vo->pixclk = p_lcd->timing.pixel_clock;
		lcd_panel_cap = p_lcd->capability;
	}
	lcd_panel_on = 1;
	
	// set lcd transmitter
	get_lcd_setting();
	transmitter_enable();

	return 0;
}

vout_dev_t lcd_vout_dev_ops = {
	.name = "LCD",
	.mode = VOUT_INF_DVI,
	.capability = VOUT_DEV_CAP_FIX_RES + VOUT_DEV_CAP_FIX_PLUG,

	.init = lcd_init,
	.set_power_down = lcd_set_power_down,
	.set_mode = lcd_set_mode,
	.config = lcd_config,
	.check_plugin = lcd_check_plugin,
	.get_edid = lcd_get_edid,
};


int lcd_module_init(void)
{
	vout_device_register(&lcd_vout_dev_ops);
	return 0;
} /* End of lcd_module_init */
module_init(lcd_module_init);
/*--------------------End of Function Body -----------------------------------*/
#undef LCD_C
