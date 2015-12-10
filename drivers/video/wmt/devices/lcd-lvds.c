/*++ 
 * linux/drivers/video/wmt/lcd-oem.c
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
 * VIA Embedded.
 * 
--*/

#define LCD_LVDS_C

#include "../lcd.h"

static void lcd_lvds_initial(void);

// LCD panel timings
lcd_parm_t lcd_lvds_parm_800x480_60 = {
	.name = "LVDS 800x480",			
	.fps = 60,						
	.bits_per_pixel = 24,
	.capability = 0,
	.timing = {
		.pixel_clock = 33500000,	
		.option = 0,				

		.hsync = 10,				
		.hbp = 89,					
		.hpixel = 800,				
		.hfp = 164,					

		.vsync = 10,					
		.vbp = 23,					
		.vpixel = 480,				
		.vfp = 10,					
	},
	.initial = lcd_lvds_initial,		
};

lcd_parm_t lcd_lvds_parm_800x480_48 = {
	.name = "LVDS 800x480",			// CHILIN LW080AT111
	.fps = 48,						/* frame per second */
	.bits_per_pixel = 18,
	.capability = 0,
	.timing = {
		.pixel_clock = 27000000,	/* pixel clock */
		.option = 0,				/* option flags */

		.hsync = 10,				/* horizontal sync pulse */
		.hbp = 50,					/* horizontal back porch */
		.hpixel = 800,				/* horizontal pixel */
		.hfp = 50,					/* horizontal front porch */

		.vsync = 5,					/* vertical sync pulse */
		.vbp = 17,					/* vertical back porch */
		.vpixel = 480,				/* vertical pixel */
		.vfp = 16,					/* vertical front porch */
	},
	.initial = lcd_lvds_initial,		
};
lcd_parm_t lcd_lvds_parm_800x600_60 = {
	.name = "LVDS 800x600",
	.fps = 60,						/* frame per second */
	.bits_per_pixel = 24,
	.capability = 0,
	.timing = {
		.pixel_clock = 40000000,	/* pixel clock */
		.option = 0,				/* option flags */

		.hsync = 128,				/* horizontal sync pulse */
		.hbp = 88,					/* horizontal back porch */
		.hpixel = 800,				/* horizontal pixel */
		.hfp = 40,					/* horizontal front porch */

		.vsync = 3,					/* vertical sync pulse */
		.vbp = 24,					/* vertical back porch */
		.vpixel = 600,				/* vertical pixel */
		.vfp = 1,					/* vertical front porch */
	},
	
	.initial = lcd_lvds_initial,		
};

lcd_parm_t lcd_lvds_parm_1024x600_60 = {
	.name = "LVDS 1024x600",			// HannStar HSD070PFW3
	.fps = 60,						/* frame per second */
	.bits_per_pixel = 24,
	.capability = 0,
	.timing = {
		.pixel_clock = 45000000,	/* pixel clock */
		.option = 0,				/* option flags */

		.hsync = 4,				/* horizontal sync pulse */
		.hbp = 50,					/* horizontal back porch */
		.hpixel = 1024,				/* horizontal pixel */
		.hfp = 50,					/* horizontal front porch */

		.vsync = 4,					/* vertical sync pulse */
		.vbp = 10,					/* vertical back porch */
		.vpixel = 600,				/* vertical pixel */
		.vfp = 10,					/* vertical front porch */
	},
	.initial = lcd_lvds_initial,		
};
lcd_parm_t lcd_lvds_parm_1024x768_60 = {
	.name = "LVDS 1024x768",
	.fps = 60,						/* frame per second */
	.bits_per_pixel = 24,
	.capability = 0,
	.timing = {
		.pixel_clock = 63500000,	/* pixel clock */
		.option = 0,				/* option flags */

		.hsync = 104,				/* horizontal sync pulse */
		.hbp = 152,					/* horizontal back porch */
		.hpixel = 1024,				/* horizontal pixel */
		.hfp = 48,					/* horizontal front porch */

		.vsync = 4,					/* vertical sync pulse */
		.vbp = 23,					/* vertical back porch */
		.vpixel = 768,				/* vertical pixel */
		.vfp = 3,					/* vertical front porch */

	},
	
	.initial = lcd_lvds_initial,		
};

lcd_parm_t lcd_lvds_parm_1366x768_60 = {
	.name = "LVDS 1366X768",			// CHIMEI or AUO 1366x768
	.fps = 60,						/* frame per second */
	.bits_per_pixel = 18,
	.capability = 0,
	.timing = {
		.pixel_clock = 75440000,	/* pixel clock */
		.option = 0,				/* option flags */

		.hsync = 65,				/* horizontal sync pulse */
		.hbp = 98,					/* horizontal back porch */
		.hpixel = 1366,				/* horizontal pixel */
		.hfp = 31,					/* horizontal front porch */

		.vsync = 12,					/* vertical sync pulse */
		.vbp = 22,					/* vertical back porch */
		.vpixel = 768,				/* vertical pixel */
		.vfp = 4,					/* vertical front porch */
	},
	.initial = lcd_lvds_initial,		
};
lcd_parm_t lcd_lvds_parm_1680x1050_60 = {
        .name = "LVDS 1680x1050",
        .fps = 60,
        .bits_per_pixel = 24,
        .capability = 0,
        .timing = {
                .pixel_clock = 146760000,
                .option = 0,

                .hsync = 176,
                .hbp = 280,
                .hpixel = 1680,
                .hfp = 104,

                .vsync = 6,
                .vbp = 30,
                .vpixel = 1050,
                .vfp = 3,
        },
        .initial = lcd_lvds_initial,
};


static void lcd_lvds_initial(void)
{	
	DPRINT("lcd_lvds_initial\n");
}
lcd_parm_t *lcd_lvds_get_parm(int arg)
{
	return &lcd_lvds_parm_800x480_60;
}
lcd_parm_t *lcd_get_lvds_parm(int resx,int resy,int fps)
{
	lcd_parm_t *lvds_parm[] = {
		&lcd_lvds_parm_800x480_60,
		&lcd_lvds_parm_800x480_48,
		&lcd_lvds_parm_800x600_60,
		&lcd_lvds_parm_1024x600_60,
		&lcd_lvds_parm_1024x768_60,
		&lcd_lvds_parm_1366x768_60,
		&lcd_lvds_parm_1680x1050_60,
		0
	};
	
	lcd_parm_t *p = 0;
	int i;

	for(i=0;;i++){
		p = lvds_parm[i];

		if( !p || (resx == p->timing.hpixel) && (resy == p->timing.vpixel) && (fps == p->fps))
			break;
	}
	
	return p;
}

int lcd_lvds_init(void){	
	int ret;	

	ret = lcd_panel_register(LCD_LVDS,(void *) lcd_lvds_get_parm);	
	return ret;
} 

module_init(lcd_lvds_init);


#undef LCD_LVDS_C
