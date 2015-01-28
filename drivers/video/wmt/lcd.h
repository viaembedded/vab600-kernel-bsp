/*++ 
 * linux/drivers/video/wmt/lcd.h
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

#ifndef LCD_H
/* To assert that only one occurrence is included */
#define LCD_H
/*-------------------- MODULE DEPENDENCY -------------------------------------*/
#include "vpp.h"

/*	following is the C++ header	*/
#ifdef	__cplusplus
extern	"C" {
#endif

/*-------------------- EXPORTED PRIVATE CONSTANTS ----------------------------*/
/* #define  LCD_XXXX  1    *//*Example*/

/*-------------------- EXPORTED PRIVATE TYPES---------------------------------*/
/* typedef  void  lcd_xxx_t;  *//*Example*/
typedef enum {
	LCD_LVDS,
	LCD_TTL,
	LCD_OEM, //There is no parameter table for OEM panel, it will give in wmt.display.param and wmt.display.tmr
	LCD_PANEL_MAX
} lcd_panel_t;

#define LCD_CAP_CLK_HI		BIT(0)
#define LCD_CAP_HSYNC_HI	BIT(1)
#define LCD_CAP_VSYNC_HI	BIT(2)
#define LCD_CAP_DE_LO		BIT(3)
typedef struct {
	char *name;
	int fps;
	int bits_per_pixel;
	unsigned int capability;

	vpp_timing_t timing;

	void (*initial)(void);
	void (*uninitial)(void);
} lcd_parm_t;

/*--------------------------------CH7305--------------------------------------*/
/** *************************************************
 * Add for CH7305
 * JerryWang(VIA Embedded)
 * 2013/03/20
 *****************************************************
 */
#define EN_DITHERING	BIT(0)
#define	DUAL_CHANNEL	BIT(1)
#define	LDI		BIT(2)
#define	OUTPUT_24	BIT(3)

typedef struct{
        unsigned char panel_mode; // single channel:0, dual channel:1
	unsigned char deskew_xcmd;
}lcd_setting_t;

static lcd_setting_t lcd_setting;

typedef struct lcd_transmitter_s{
	struct lcd_transmitter_s *next;
	char *name;
	int (*init)(void);
	void (*power_on)(void);
	void (*power_off)(void);
	void (*suspend)(void);
	void (*resume)(void);
	int (*set_mode)(lcd_setting_t mode);
}lcd_transmitter_t;

int lcd_transmitter_register(lcd_transmitter_t *ops);

/*----------------------------------------------------------------------------*/


/*-------------------- EXPORTED PRIVATE VARIABLES -----------------------------*/
#ifdef LCD_C /* allocate memory for variables only in vout.c */
#define EXTERN
#else
#define EXTERN   extern
#endif /* ifdef LCD_C */

/* EXTERN int      lcd_xxx; *//*Example*/

EXTERN lcd_parm_t *p_lcd;
#undef EXTERN

/*--------------------- EXPORTED PRIVATE MACROS -------------------------------*/
/* #define LCD_XXX_YYY   xxxx *//*Example*/
/*--------------------- EXPORTED PRIVATE FUNCTIONS  ---------------------------*/
/* extern void  lcd_xxx(void); *//*Example*/

int lcd_panel_register(int no,void (*get_parm)(int mode));
lcd_parm_t *lcd_get_parm(lcd_panel_t id,unsigned int arg);
lcd_parm_t *lcd_get_oem_parm(int resx,int resy);
lcd_parm_t *lcd_get_lvds_parm(int resx,int resy, int fps);
lcd_parm_t *lcd_get_ttl_parm(int resx,int resy, int fps);
//void lcd_set_parm(int id,int bpp);
void lcd_set_parm(int ops, int bpp, int hpixel, int vpixel, int fps);
void lcd_set_lvds_id(int id);
int lcd_get_lvds_id(void);
void lcd_suspend(void);
void lcd_resume(void);

/* LCD back light */
#ifdef	__cplusplus
}
#endif	
#endif /* ifndef LCD_H */
