/*++ 
 * linux/drivers/video/wmt/ch7305.c
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
 * VIA Embedded, Inc.
 --*/
#define CH7305_C

#include "../vout.h"
#include "../lcd.h"
#include <linux/delay.h>

#define CH7305_ADDR 		0xEA
#define CH7305_ID          	0x81
#define CH7305_I2C_ID		0x0 		//I2C0
#define CH7305_REG_COUNT	0x80

unsigned int regs[CH7305_REG_COUNT];

typedef struct{
	unsigned char addr;
	unsigned char data;
}reg_t;

extern void PWM0_GPIO_Output_Without_Frequecny_Control(int enable);

int ve_ch7305_init()
{

#ifdef CONFIG_PWM0_GPIO_Output_Without_Frequency_Control
	/* Disable PWM0 frequency output to avoid CH7305 I20 hang situation */
	PWM0_GPIO_Output_Without_Frequecny_Control(1);
#endif
	reg_t id = {0x4A, 0xFF};

	vpp_i2c_release();
	vpp_i2c_init(CH7305_I2C_ID, CH7305_ADDR);

	vpp_i2c_read(CH7305_I2C_ID, CH7305_ADDR, id.addr, &id.data, 1);

	// CH7305 version ID is 0x81
	if( id.data!= CH7305_ID){
		//DPRINT("[CH7305] Err: Fail to detect\n");
		return -1;
	}
	
	DPRINT("CH7305 detected\n");
	return 0;
}
void ve_ch7305_power_on()
{
	// LVDS power on
	reg_t reg63;
	reg63.addr = 0x63;
	vpp_i2c_read(CH7305_I2C_ID, CH7305_ADDR, reg63.addr, &reg63.data, 1);
	reg63.data &= ~0x40; 
	vpp_i2c_write(CH7305_I2C_ID, CH7305_ADDR, reg63.addr, &reg63.data, 1);

	/* add for CH7305 PLL reset to prevent CH7305 from no signal ouput */
	reg_t reg76;
	reg76.addr = 0x76;

	reg_t reg66;
	reg66.addr = 0x66;

	int timeout = 0;
	int ret=0;

	/* wait until CH7305 PLL is stable or timeout */
	while( ((ret=vpp_i2c_read(CH7305_I2C_ID, CH7305_ADDR, reg66.addr, &reg66.data, 1))>0) && !(reg66.data & 0x04) && (timeout++)<3 ) 
	{
//		printk("ret=%d, reg66.data=0x%x, reg66.data & 0x04=%d, timeout=%d\n",ret,reg66.data, reg66.data & 0x04, timeout);
		vpp_i2c_read(CH7305_I2C_ID, CH7305_ADDR, reg76.addr, &reg76.data, 1);
		reg76.data &= ~0x04; 
		vpp_i2c_write(CH7305_I2C_ID, CH7305_ADDR, reg76.addr, &reg76.data, 1);
		mdelay(100);
		reg76.data |= 0x04; 
		vpp_i2c_write(CH7305_I2C_ID, CH7305_ADDR, reg76.addr, &reg76.data, 1);
		mdelay(100);
	}
	
#if 0
	// GPIO0 and GPIO1 control backlight and power
	vppif_reg32_write(GPIO_BASE_ADDR+0x80, 0x3, 0, 0x3);
	mdelay(10);
	vppif_reg32_write(GPIO_BASE_ADDR+0xC0, 0x3, 0, 0x3);
	mdelay(10);		
#endif
}
void ve_ch7305_power_off()
{
	// LVDS power off
	reg_t reg63;
	reg63.addr = 0x63;
	vpp_i2c_read(CH7305_I2C_ID, CH7305_ADDR, reg63.addr, &reg63.data, 1);
	reg63.data |= 0x40; 
	vpp_i2c_write(CH7305_I2C_ID, CH7305_ADDR, reg63.addr, &reg63.data, 1);
	
#if 0
	// GPIO0 and GPIO1
	vppif_reg32_write(GPIO_BASE_ADDR+0x80, 0x3, 0, 0);
	mdelay(10);
	vppif_reg32_write(GPIO_BASE_ADDR+0xC0, 0x3, 0, 0);
	mdelay(10);
#endif
}

void print_reg(){
	reg_t reg;
	int i,j;
	DPRINT("********************************************\n");
	DPRINT("[CH7305] Register:\n");
	for(i = 0; i < 8; i++){
		for(j = 0; j < 16; j++){
			reg.addr = i*16 +j;
			reg.data = 0xff;
			vpp_i2c_read(CH7305_I2C_ID, CH7305_ADDR, reg.addr, &reg.data, 1);
			DPRINT("%x\t",reg.data);			
		}
		DPRINT("\n");
	}
	DPRINT("*********************************************\n");

}
int ve_ch7305_set_mode(lcd_setting_t mode)
{
	/* Channel and mods:
	  * Single channel: 800x600, 1024x768
	  * dual channel:  1280x1024, 1400x1050, 1600x1200
	  */
	reg_t pll_single[] = {
		{0x6F, 0x00},
		{0x70, 0x40},
		{0x71, 0xED},
		{0x72, 0xA3},
		{0x73, 0xC8},
		{0x74, 0xF6},
		{0x75, 0x00},
		{0x76, 0xAD},
		{0x77, 0x00},
		{0x78, 0x80},
		{0x79, 0x02},
		{0x7A, 0x00},
		{0x7B, 0x00},
		{0x7C, 0x30},
		{0x7D, 0x02},
		{0x7E, 0x00},
		{0x7F, 0x10}
	};
	reg_t pll_dual[] = {
		{0x6F, 0x00},
		{0x70, 0x40},
		{0x71, 0xE3},
		{0x72, 0xAD},
		{0x73, 0xDB},
		{0x74, 0xF6},
		{0x75, 0x00},
		{0x76, 0x8F},
		{0x77, 0x00},
		{0x78, 0x80},
		{0x79, 0x02},
		{0x7A, 0x00},
		{0x7B, 0x00},
		{0x7C, 0x30},
		{0x7D, 0x02},
		{0x7E, 0x00},
		{0x7F, 0x10}
	};
	
	// Print registers' value
	//	print_reg();
	int i;
	for (i = 0; i < 17; i++ ){
		if ( mode.panel_mode & DUAL_CHANNEL )
			vpp_i2c_write(CH7305_I2C_ID, CH7305_ADDR, pll_dual[i].addr, &pll_dual[i].data, 1);
		else
			vpp_i2c_write(CH7305_I2C_ID, CH7305_ADDR, pll_single[i].addr, &pll_single[i].data, 1);
	}
	
	reg_t reg64;
	reg64.addr = 0x64;
	
	// Input data select
	vpp_i2c_read(CH7305_I2C_ID, CH7305_ADDR, reg64.addr, &reg64.data, 1);
	if ( mode.panel_mode & DUAL_CHANNEL )
		reg64.data |= 0x10;
	else
		reg64.data &= ~0x10;
	vpp_i2c_write(CH7305_I2C_ID, CH7305_ADDR, reg64.addr, &reg64.data, 1);

	//Power sequence
	reg_t T1,T2, T3, T4, T5;
	T1.addr =0x67;
	T2.addr =0x68;
	T3.addr =0x69;
	T4.addr =0x6A;
	T5.addr =0x6B;
	T1.data = 0x31;
	T2.data = 0x68;
	T3.data = 0x68;
	T4.data = 0x31;
	T5.data = 0x4;
	unsigned char val;
	// T1(Power On Time)
	vpp_i2c_write(CH7305_I2C_ID, CH7305_ADDR, T1.addr, &T1.data, 1);
	val = T2.data & 0x80;
	vpp_i2c_write(CH7305_I2C_ID, CH7305_ADDR, T2.addr, &val, 1);
	// T4(Power Off Time)	
	vpp_i2c_write(CH7305_I2C_ID, CH7305_ADDR, T4.addr, &T4.data, 1);
	val = T3.data & 0x80;
	vpp_i2c_write(CH7305_I2C_ID, CH7305_ADDR, T3.addr, &val, 1);
	// T5(Power Cycle Time)
	val = T5.data & 0x3F;
	vpp_i2c_write(CH7305_I2C_ID, CH7305_ADDR, T5.addr, &val, 1);
	// T2(Black Light Enable Time)
	val = T2.data & 0x7F;
	vpp_i2c_write(CH7305_I2C_ID, CH7305_ADDR, T2.addr, &val, 1);
	// T3(Black Light Disable Time)	
	val = T3.data & 0x7F;
	vpp_i2c_write(CH7305_I2C_ID, CH7305_ADDR, T3.addr, &val, 1);

	// LVDS24
	if (  mode.panel_mode & OUTPUT_24 )
		reg64.data |= 0x20;				// Output 24bits
	else
		reg64.data &= ~0x20;				// Output 18bits

	// Dithering
	if ( mode.panel_mode & EN_DITHERING )
		reg64.data &= ~0x08;				// Enable dithering
	else
		reg64.data |= 0x08;				// Disable dithering
	
	// LDI
	if ( mode.panel_mode & LDI)
		reg64.data |= 0x1;				// openLDI
	else
		reg64.data &= ~0x1;				// SPWG
	
	vpp_i2c_write(CH7305_I2C_ID, CH7305_ADDR, reg64.addr, &reg64.data, 1);

	// De-skew
	reg_t reg1d;
	reg1d.addr = 0x1d;
	reg1d.data = 0x40;
	reg1d.data |= mode.deskew_xcmd & 0xF;
	vpp_i2c_write(CH7305_I2C_ID, CH7305_ADDR, reg1d.addr, &reg1d.data, 1);
	
	//print registers' value
	//print_reg();
	
	return 0;
}
void ve_ch7305_suspend(void){
	
	int i;
	for(i = 0; i < CH7305_REG_COUNT; i++)
		vpp_i2c_read(CH7305_I2C_ID, CH7305_ADDR, i, &regs[i], 1);
	
}
void ve_ch7305_resume(void){
	int i;
	for(i = 0; i < CH7305_REG_COUNT; i++)
		vpp_i2c_write(CH7305_I2C_ID, CH7305_ADDR, i, &regs[i], 1);

}

/*----------------------- vout device plugin --------------------------------------*/
lcd_transmitter_t ch7305_lcd_transmitter_ops = {
	.name = "CH7305",		
	.init = ve_ch7305_init,
	.set_mode = ve_ch7305_set_mode,
	.power_on = ve_ch7305_power_on,
	.power_off = ve_ch7305_power_off,
	.suspend = ve_ch7305_suspend,
	.resume = ve_ch7305_resume,
};

static int ch7305_module_init(void)
{	
	lcd_transmitter_register(&ch7305_lcd_transmitter_ops);
	return 0;
} 
module_init(ch7305_module_init);
#undef CH7305_C

