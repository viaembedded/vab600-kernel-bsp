/*++ 
 * linux/drivers/video/wmt/lcd-INNOLUX-AT070TN83.c
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
 * VIA Embedded
 * 2013/02/19
--*/

#define LCD_TP070C01_C
#include "../lcd.h"


static void lcd_tp070c01_power_on(void);
static void lcd_tp070c01_power_off(void);

// Panel Timing
lcd_parm_t lcd_tp070c01_parm = {
	.name = "VE TP070C01",
	.fps = 60,					/* frame per second */
	.bits_per_pixel = 24,
	//.capability = LCD_CAP_CLK_HI,
	.capability = 0,
	.timing = {
		.pixel_clock = 33500000,	/* pixel clock */
		.option = 0,				/* option flags */

		.hsync = 10,				/* horizontal sync pulse */
		.hbp = 89,				/* horizontal back porch */
		.hpixel = 800,				/* horizontal pixel */
		.hfp = 164,				/* horizontal front porch */

		.vsync = 10,				/* vertical sync pulse */
		.vbp = 23,				/* vertical back porch */
		.vpixel = 480,				/* vertical pixel */
		.vfp = 10,				/* vertical front porch */
	},
	
	.initial = lcd_tp070c01_power_on,
	.uninitial = lcd_tp070c01_power_off,
};

static void lcd_tp070c01_power_on(void)
{	 
	// GPIO0 and GPIO1 control backlight
	/*
	vppif_reg32_write(GPIO_BASE_ADDR+0x80, 0x3, 0, 0x3);
	mdelay(10);
	vppif_reg32_write(GPIO_BASE_ADDR+0xC0, 0x3, 0, 0x3);
	mdelay(10);
	*/
}

static void lcd_tp070c01_power_off(void)
{	
	/*
	vppif_reg32_write(GPIO_BASE_ADDR+0x80, 0x3, 0, 0);
	mdelay(10);
	vppif_reg32_write(GPIO_BASE_ADDR+0xC0, 0x3, 0, 0);
	mdelay(10);
	*/
}

lcd_parm_t *lcd_tp070c01_get_parm(int arg) 
{	
	return &lcd_tp070c01_parm;
}

int lcd_tp070c01_init(void){
	int ret;	

	ret = lcd_panel_register(LCD_TP070C01,(void *) lcd_tp070c01_get_parm);	
	return ret;
} 
module_init(lcd_tp070c01_init);


#undef LCD_TP070C01_C

