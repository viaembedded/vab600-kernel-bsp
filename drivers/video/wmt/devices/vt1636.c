/*++ 
 * linux/drivers/video/wmt/vt1636.c
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
#define vt1636_C

#include "../vout.h"
#include "../lcd.h"
#include <linux/delay.h>

#define vt1636_ADDR 		0x80
#define vt1636_ID_H          	0x11
#define vt1636_ID_L          	0x06
#define VT1636_I2C_ID		0x0 		//I2C0
#define VT1636_REG_COUNT	0x16

unsigned int vt1636_regs[VT1636_REG_COUNT];

typedef struct{
	unsigned char addr;
	unsigned char data;
}vt1636_reg_t;

extern void PWM0_GPIO_Output_Without_Frequecny_Control(int enable);

int vt1636_init()
{
	//printk("[vt1636]: %s\n",__func__);
#ifdef CONFIG_PWM0_GPIO_Output_Without_Frequency_Control
	/* Disable PWM0 frequency output to avoid vt1636 I20 hang situation */
	PWM0_GPIO_Output_Without_Frequecny_Control(1);
#endif
	vt1636_reg_t id_h = {0x01, 0xFF};
	vt1636_reg_t id_l = {0x00, 0xFF};

	vpp_i2c_release();
	vpp_i2c_init(VT1636_I2C_ID, vt1636_ADDR);

	vpp_i2c_read(VT1636_I2C_ID, vt1636_ADDR, id_h.addr, &id_h.data, 1);
	vpp_i2c_read(VT1636_I2C_ID, vt1636_ADDR, id_l.addr, &id_l.data, 1);

	// vt1636 version ID is 0x0611
	if( id_h.data!= vt1636_ID_H || id_l.data != vt1636_ID_L){
		//DPRINT("[vt1636] Err: Fail to detect\n");
		return -1;
	}
	
	DPRINT("VT1636 detected\n");
	return 0;
}
void vt1636_power_on()
{
	//printk("[vt1636]: %s\n",__func__);
	// LVDS power on
	vt1636_reg_t reg9,reg10;
	reg9.addr = 0x09;
	vpp_i2c_read(VT1636_I2C_ID, vt1636_ADDR, reg9.addr, &reg9.data, 1);
	reg9.data |= 0x80; 
	vpp_i2c_write(VT1636_I2C_ID, vt1636_ADDR, reg9.addr, &reg9.data, 1);

	
}
void vt1636_power_off()
{
	// LVDS power off
	//printk("[vt1636]: %s\n",__func__);
	vt1636_reg_t reg9,reg10;
	reg9.addr = 0x09;
	vpp_i2c_read(VT1636_I2C_ID, vt1636_ADDR, reg9.addr, &reg9.data, 1);
	reg9.data &= ~0x80; 
	vpp_i2c_write(VT1636_I2C_ID, vt1636_ADDR, reg9.addr, &reg9.data, 1);

}

void vt1636_print_reg(){
	vt1636_reg_t reg;
	int i,j;
	DPRINT("********************************************\n");
	DPRINT("[vt1636] Register:\n");
	for(i = 0; i < 2; i++){
		for(j = 0; j < 11; j++){
			reg.addr = i*11 +j;
			reg.data = 0xff;
			vpp_i2c_read(VT1636_I2C_ID, vt1636_ADDR, reg.addr, &reg.data, 1);
			DPRINT("%x\t",reg.data);			
		}
		DPRINT("\n");
	}
	DPRINT("*********************************************\n");

}
int vt1636_set_mode(lcd_setting_t mode)
{
	//printk("[vt1636]: %s\n",__func__);
	vt1636_reg_t reg8,reg9;
	reg8.addr = 0x08;
	reg9.addr = 0x09;
	
	// select the channel
	vpp_i2c_read(VT1636_I2C_ID, vt1636_ADDR, reg8.addr, &reg8.data, 1);
	if ( mode.panel_mode & DUAL_CHANNEL )
		reg8.data |= 0xe0;
	else
		reg8.data &= ~0xe0;
	// select the output
	if (  mode.panel_mode & OUTPUT_24 )
		reg8.data |= 0x10;				// Output 24bits
	else
		reg8.data &= ~0x10;				// Output 18bits
	vpp_i2c_write(VT1636_I2C_ID, vt1636_ADDR, reg8.addr, &reg8.data, 1);

	/*config the pll to resolve the spiral screen*/
	vpp_i2c_read(VT1636_I2C_ID, vt1636_ADDR, reg9.addr, &reg9.data, 1);
	reg9.data &= 0xf0;
	reg9.data |= 0x0a;
	vpp_i2c_write(VT1636_I2C_ID, vt1636_ADDR, reg9.addr, &reg9.data, 1);

	//vt1636_print_reg();
	
	return 0;
}
void vt1636_suspend(void){
	//printk("[vt1636]: %s\n",__func__);	
	int i;
	for(i = 0; i < VT1636_REG_COUNT; i++)
		vpp_i2c_read(VT1636_I2C_ID, vt1636_ADDR, i, &vt1636_regs[i], 1);
	
}
void vt1636_resume(void){
	//printk("[vt1636]: %s\n",__func__);
	int i;
	for(i = 0; i < VT1636_REG_COUNT; i++)
		vpp_i2c_write(VT1636_I2C_ID, vt1636_ADDR, i, &vt1636_regs[i], 1);

}

/*----------------------- vout device plugin --------------------------------------*/
lcd_transmitter_t vt1636_lcd_transmitter_ops = {
	.name = "vt1636",		
	.init = vt1636_init,
	.set_mode = vt1636_set_mode,
	.power_on = vt1636_power_on,
	.power_off = vt1636_power_off,
	.suspend = vt1636_suspend,
	.resume = vt1636_resume,
};

static int vt1636_module_init(void)
{	
	lcd_transmitter_register(&vt1636_lcd_transmitter_ops);
	return 0;
} 

module_init(vt1636_module_init);
MODULE_DESCRIPTION("vt1636 driver");
MODULE_AUTHOR("RolandZheng@viateck.com.cn");
#undef vt1636_C

