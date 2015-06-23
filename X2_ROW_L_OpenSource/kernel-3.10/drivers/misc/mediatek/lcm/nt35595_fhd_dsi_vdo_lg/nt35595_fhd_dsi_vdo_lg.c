/* BEGIN PN:DTS2013053103858 , Added by d00238048, 2013.05.31*/
#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
	#include <platform/mt_i2c.h> 
	#include <platform/mt_pmic.h>
#elif defined(BUILD_UBOOT)
    #include <asm/arch/mt_gpio.h>
#else
    //#include <linux/delay.h>
    #include <mach/mt_gpio.h>
#endif
#include <cust_gpio_usage.h>
#include <cust_i2c.h>
#ifdef BUILD_LK
#define LCD_DEBUG(fmt)  dprintf(CRITICAL,fmt)
#else
#define LCD_DEBUG(fmt)  printk(fmt)
#endif

//extern int isAAL;
//#define GPIO_AAL_ID		 (GPIO174 | 0x80000000)
static const unsigned int BL_MIN_LEVEL =20;
static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))
#define MDELAY(n) 											(lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmd_by_cmdq(handle,cmd,count,ppara,force_update)    lcm_util.dsi_set_cmdq_V22(handle,cmd,count,ppara,force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)										lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   			lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)    


//static unsigned char lcd_id_pins_value = 0xFF;
static const unsigned char LCD_MODULE_ID = 0x01; //  haobing modified 2013.07.11
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define LCM_DSI_CMD_MODE									1
#define FRAME_WIDTH  										(1080)
#define FRAME_HEIGHT 										(1920)


#define REGFLAG_DELAY             								0xFC
#define REGFLAG_END_OF_TABLE      							0xFD   // END OF REGISTERS MARKER


#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif
//static unsigned int lcm_esd_test = FALSE;      ///only for ESD test
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------


struct LCM_setting_table {
    unsigned char cmd;
    unsigned char count;
    unsigned char para_list[64];
};

/* lenovo-sw yexm1 add lcd effect function 20140314 start */
LCM_DSI_MODE_SWITCH_CMD lcm_switch_mode_cmd;

#ifndef BUILD_LK
#ifdef CONFIG_LENOVO_LCM_EFFECT
static unsigned int lcm_gammamode_index = 0;
#include "nt35595_lg_CT.h"
#include "nt35595_lg_CE.h"
#include "nt35595_lg_SC.h"

static struct LCM_setting_table lcm_gamma_cosy_setting[] = {
//page selection cmd start
{0xFF,1,{0x20}},
//page selecti}},on cmd end
{REGFLAG_DELAY, 2, {}},
//R(+) MCR c}},md
{0x75,1,{0x00}},
{0x76,1,{0x01}},
{0x77,1,{0x00}},
{0x78,1,{0x02}},
{0x79,1,{0x00}},
{0x7A,1,{0x09}},
{0x7B,1,{0x00}},
{0x7C,1,{0x2A}},
{0x7D,1,{0x00}},
{0x7E,1,{0x3E}},
{0x7F,1,{0x00}},
{0x80,1,{0x4C}},
{0x81,1,{0x00}},
{0x82,1,{0x61}},
{0x83,1,{0x00}},
{0x84,1,{0x70}},
{0x85,1,{0x00}},
{0x86,1,{0x7C}},
{0x87,1,{0x00}},
{0x88,1,{0xAB}},
{0x89,1,{0x00}},
{0x8A,1,{0xD2}},
{0x8B,1,{0x01}},
{0x8C,1,{0x0E}},
{0x8D,1,{0x01}},
{0x8E,1,{0x43}},
{0x8F,1,{0x01}},
{0x90,1,{0x90}},
{0x91,1,{0x01}},
{0x92,1,{0xD1}},
{0x93,1,{0x01}},
{0x94,1,{0xD2}},
{0x95,1,{0x02}},
{0x96,1,{0x0F}},
{0x97,1,{0x02}},
{0x98,1,{0x50}},
{0x99,1,{0x02}},
{0x9A,1,{0x7A}},
{0x9B,1,{0x02}},
{0x9C,1,{0xB1}},
{0x9D,1,{0x02}},
{0x9E,1,{0xD5}},
{0x9F,1,{0x03}},
{0xA0,1,{0x02}},
{0xA2,1,{0x03}},
{0xA3,1,{0x12}},
{0xA4,1,{0x03}},
{0xA5,1,{0x24}},
{0xA6,1,{0x03}},
{0xA7,1,{0x39}},
{0xA9,1,{0x03}},
{0xAA,1,{0x4F}},
{0xAB,1,{0x03}},
{0xAC,1,{0x6A}},
{0xAD,1,{0x03}},
{0xAE,1,{0x8A}},
{0xAF,1,{0x03}},
{0xB0,1,{0xEC}},
{0xB1,1,{0x03}},
{0xB2,1,{0xFD}},
//R(-) MCR cm}},d
{0xB3,1,{0x00}},
{0xB4,1,{0x01}},
{0xB5,1,{0x00}},
{0xB6,1,{0x02}},
{0xB7,1,{0x00}},
{0xB8,1,{0x09}},
{0xB9,1,{0x00}},
{0xBA,1,{0x2A}},
{0xBB,1,{0x00}},
{0xBC,1,{0x3E}},
{0xBD,1,{0x00}},
{0xBE,1,{0x4C}},
{0xBF,1,{0x00}},
{0xC0,1,{0x61}},
{0xC1,1,{0x00}},
{0xC2,1,{0x70}},
{0xC3,1,{0x00}},
{0xC4,1,{0x7C}},
{0xC5,1,{0x00}},
{0xC6,1,{0xAB}},
{0xC7,1,{0x00}},
{0xC8,1,{0xD2}},
{0xC9,1,{0x01}},
{0xCA,1,{0x0E}},
{0xCB,1,{0x01}},
{0xCC,1,{0x43}},
{0xCD,1,{0x01}},
{0xCE,1,{0x90}},
{0xCF,1,{0x01}},
{0xD0,1,{0xD1}},
{0xD1,1,{0x01}},
{0xD2,1,{0xD2}},
{0xD3,1,{0x02}},
{0xD4,1,{0x0F}},
{0xD5,1,{0x02}},
{0xD6,1,{0x50}},
{0xD7,1,{0x02}},
{0xD8,1,{0x7A}},
{0xD9,1,{0x02}},
{0xDA,1,{0xB1}},
{0xDB,1,{0x02}},
{0xDC,1,{0xD5}},
{0xDD,1,{0x03}},
{0xDE,1,{0x02}},
{0xDF,1,{0x03}},
{0xE0,1,{0x12}},
{0xE1,1,{0x03}},
{0xE2,1,{0x24}},
{0xE3,1,{0x03}},
{0xE4,1,{0x39}},
{0xE5,1,{0x03}},
{0xE6,1,{0x4F}},
{0xE7,1,{0x03}},
{0xE8,1,{0x6A}},
{0xE9,1,{0x03}},
{0xEA,1,{0x8A}},
{0xEB,1,{0x03}},
{0xEC,1,{0xEC}},
{0xED,1,{0x03}},
{0xEE,1,{0xFD}},
//G(+) MCR c}},md
{0xEF,1,{0x00}},
{0xF0,1,{0x92}},
{0xF1,1,{0x00}},
{0xF2,1,{0x99}},
{0xF3,1,{0x00}},
{0xF4,1,{0xA6}},
{0xF5,1,{0x00}},
{0xF6,1,{0xAF}},
{0xF7,1,{0x00}},
{0xF8,1,{0xB9}},
{0xF9,1,{0x00}},
{0xFA,1,{0xC2}},
//page selecti}},on cmd start
{0xFF,1,{0x21}},
{REGFLAG_DELAY, 2, {}},
//page selecti}},on cmd end
{0x00,1,{0x00}},
{0x01,1,{0xCA}},
{0x02,1,{0x00}},
{0x03,1,{0xD2}},
{0x04,1,{0x00}},
{0x05,1,{0xDB}},
{0x06,1,{0x00}},
{0x07,1,{0xF8}},
{0x08,1,{0x01}},
{0x09,1,{0x12}},
{0x0A,1,{0x01}},
{0x0B,1,{0x41}},
{0x0C,1,{0x01}},
{0x0D,1,{0x67}},
{0x0E,1,{0x01}},
{0x0F,1,{0xAA}},
{0x10,1,{0x01}},
{0x11,1,{0xE3}},
{0x12,1,{0x01}},
{0x13,1,{0xE5}},
{0x14,1,{0x02}},
{0x15,1,{0x1D}},
{0x16,1,{0x02}},
{0x17,1,{0x5D}},
{0x18,1,{0x02}},
{0x19,1,{0x85}},
{0x1A,1,{0x02}},
{0x1B,1,{0xBD}},
{0x1C,1,{0x02}},
{0x1D,1,{0xE1}},
{0x1E,1,{0x03}},
{0x1F,1,{0x11}},
{0x20,1,{0x03}},
{0x21,1,{0x21}},
{0x22,1,{0x03}},
{0x23,1,{0x31}},
{0x24,1,{0x03}},
{0x25,1,{0x44}},
{0x26,1,{0x03}},
{0x27,1,{0x5B}},
{0x28,1,{0x03}},
{0x29,1,{0x74}},
{0x2A,1,{0x03}},
{0x2B,1,{0x96}},
{0x2D,1,{0x03}},
{0x2F,1,{0xCE}},
{0x30,1,{0x03}},
{0x31,1,{0xFD}},
//G(-) MCR c}},md
{0x32,1,{0x00}},
{0x33,1,{0x92}},
{0x34,1,{0x00}},
{0x35,1,{0x99}},
{0x36,1,{0x00}},
{0x37,1,{0xA6}},
{0x38,1,{0x00}},
{0x39,1,{0xAF}},
{0x3A,1,{0x00}},
{0x3B,1,{0xB9}},
{0x3D,1,{0x00}},
{0x3F,1,{0xC2}},
{0x40,1,{0x00}},
{0x41,1,{0xCA}},
{0x42,1,{0x00}},
{0x43,1,{0xD2}},
{0x44,1,{0x00}},
{0x45,1,{0xDB}},
{0x46,1,{0x00}},
{0x47,1,{0xF8}},
{0x48,1,{0x01}},
{0x49,1,{0x12}},
{0x4A,1,{0x01}},
{0x4B,1,{0x41}},
{0x4C,1,{0x01}},
{0x4D,1,{0x67}},
{0x4E,1,{0x01}},
{0x4F,1,{0xAA}},
{0x50,1,{0x01}},
{0x51,1,{0xE3}},
{0x52,1,{0x01}},
{0x53,1,{0xE5}},
{0x54,1,{0x02}},
{0x55,1,{0x1D}},
{0x56,1,{0x02}},
{0x58,1,{0x5D}},
{0x59,1,{0x02}},
{0x5A,1,{0x85}},
{0x5B,1,{0x02}},
{0x5C,1,{0xBD}},
{0x5D,1,{0x02}},
{0x5E,1,{0xE1}},
{0x5F,1,{0x03}},
{0x60,1,{0x11}},
{0x61,1,{0x03}},
{0x62,1,{0x21}},
{0x63,1,{0x03}},
{0x64,1,{0x31}},
{0x65,1,{0x03}},
{0x66,1,{0x44}},
{0x67,1,{0x03}},
{0x68,1,{0x5B}},
{0x69,1,{0x03}},
{0x6A,1,{0x74}},
{0x6B,1,{0x03}},
{0x6C,1,{0x96}},
{0x6D,1,{0x03}},
{0x6E,1,{0xCE}},
{0x6F,1,{0x03}},
{0x70,1,{0xFD}},
//B(+) MCR c}},md
{0x71,1,{0x01}},
{0x72,1,{0x10}},
{0x73,1,{0x01}},
{0x74,1,{0x11}},
{0x75,1,{0x01}},
{0x76,1,{0x17}},
{0x77,1,{0x01}},
{0x78,1,{0x1C}},
{0x79,1,{0x01}},
{0x7A,1,{0x21}},
{0x7B,1,{0x01}},
{0x7C,1,{0x26}},
{0x7D,1,{0x01}},
{0x7E,1,{0x2C}},
{0x7F,1,{0x01}},
{0x80,1,{0x30}},
{0x81,1,{0x01}},
{0x82,1,{0x35}},
{0x83,1,{0x01}},
{0x84,1,{0x47}},
{0x85,1,{0x01}},
{0x86,1,{0x58}},
{0x87,1,{0x01}},
{0x88,1,{0x79}},
{0x89,1,{0x01}},
{0x8A,1,{0x95}},
{0x8B,1,{0x01}},
{0x8C,1,{0xCA}},
{0x8D,1,{0x01}},
{0x8E,1,{0xFC}},
{0x8F,1,{0x01}},
{0x90,1,{0xFE}},
{0x91,1,{0x02}},
{0x92,1,{0x30}},
{0x93,1,{0x02}},
{0x94,1,{0x6F}},
{0x95,1,{0x02}},
{0x96,1,{0x9A}},
{0x97,1,{0x02}},
{0x98,1,{0xD8}},
{0x99,1,{0x03}},
{0x9A,1,{0x0A}},
{0x9B,1,{0x03}},
{0x9C,1,{0xC6}},
{0x9D,1,{0x03}},
{0x9E,1,{0xCB}},
{0x9F,1,{0x03}},
{0xA0,1,{0xD4}},
{0xA2,1,{0x03}},
{0xA3,1,{0xD9}},
{0xA4,1,{0x03}},
{0xA5,1,{0xE2}},
{0xA6,1,{0x03}},
{0xA7,1,{0xE8}},
{0xA9,1,{0x03}},
{0xAA,1,{0xED}},
{0xAB,1,{0x03}},
{0xAC,1,{0xF7}},
{0xAD,1,{0x03}},
{0xAE,1,{0xFD}},
//B(-) MCR cm}},d
{0xAF,1,{0x01}},
{0xB0,1,{0x10}},
{0xB1,1,{0x01}},
{0xB2,1,{0x11}},
{0xB3,1,{0x01}},
{0xB4,1,{0x17}},
{0xB5,1,{0x01}},
{0xB6,1,{0x1C}},
{0xB7,1,{0x01}},
{0xB8,1,{0x21}},
{0xB9,1,{0x01}},
{0xBA,1,{0x26}},
{0xBB,1,{0x01}},
{0xBC,1,{0x2C}},
{0xBD,1,{0x01}},
{0xBE,1,{0x30}},
{0xBF,1,{0x01}},
{0xC0,1,{0x35}},
{0xC1,1,{0x01}},
{0xC2,1,{0x47}},
{0xC3,1,{0x01}},
{0xC4,1,{0x58}},
{0xC5,1,{0x01}},
{0xC6,1,{0x79}},
{0xC7,1,{0x01}},
{0xC8,1,{0x95}},
{0xC9,1,{0x01}},
{0xCA,1,{0xCA}},
{0xCB,1,{0x01}},
{0xCC,1,{0xFC}},
{0xCD,1,{0x01}},
{0xCE,1,{0xFE}},
{0xCF,1,{0x02}},
{0xD0,1,{0x30}},
{0xD1,1,{0x02}},
{0xD2,1,{0x6F}},
{0xD3,1,{0x02}},
{0xD4,1,{0x9A}},
{0xD5,1,{0x02}},
{0xD6,1,{0xD8}},
{0xD7,1,{0x03}},
{0xD8,1,{0x0A}},
{0xD9,1,{0x03}},
{0xDA,1,{0xC6}},
{0xDB,1,{0x03}},
{0xDC,1,{0xCB}},
{0xDD,1,{0x03}},
{0xDE,1,{0xD4}},
{0xDF,1,{0x03}},
{0xE0,1,{0xD9}},
{0xE1,1,{0x03}},
{0xE2,1,{0xE2}},
{0xE3,1,{0x03}},
{0xE4,1,{0xE8}},
{0xE5,1,{0x03}},
{0xE6,1,{0xED}},
{0xE7,1,{0x03}},
{0xE8,1,{0xF7}},
{0xE9,1,{0x03}},
{0xEA,1,{0xFD}},

{0xFF,1,{0x10}},

};

static struct LCM_setting_table lcm_gamma_sunshine_setting[] = {
};

static struct LCM_setting_table lcm_gamma_default_setting[] = {
//page selection cmd start
{0xFF,1,{0x20}},
{REGFLAG_DELAY, 2, {}},
{0xFB,1,{0x01}},

//page selecti}},on cmd end
//R(+) MCR c}},md
{0x75,1,{0x00}},
{0x76,1,{0x01}},
{0x77,1,{0x00}},
{0x78,1,{0x03}},
{0x79,1,{0x00}},
{0x7A,1,{0x1D}},
{0x7B,1,{0x00}},
{0x7C,1,{0x36}},
{0x7D,1,{0x00}},
{0x7E,1,{0x49}},
{0x7F,1,{0x00}},
{0x80,1,{0x5B}},
{0x81,1,{0x00}},
{0x82,1,{0x67}},
{0x83,1,{0x00}},
{0x84,1,{0x77}},
{0x85,1,{0x00}},
{0x86,1,{0x84}},
{0x87,1,{0x00}},
{0x88,1,{0xB3}},
{0x89,1,{0x00}},
{0x8A,1,{0xD7}},
{0x8B,1,{0x01}},
{0x8C,1,{0x13}},
{0x8D,1,{0x01}},
{0x8E,1,{0x44}},
{0x8F,1,{0x01}},
{0x90,1,{0x91}},
{0x91,1,{0x01}},
{0x92,1,{0xD1}},
{0x93,1,{0x01}},
{0x94,1,{0xD4}},
{0x95,1,{0x02}},
{0x96,1,{0x10}},
{0x97,1,{0x02}},
{0x98,1,{0x51}},
{0x99,1,{0x02}},
{0x9A,1,{0x7C}},
{0x9B,1,{0x02}},
{0x9C,1,{0xB2}},
{0x9D,1,{0x02}},
{0x9E,1,{0xD7}},
{0x9F,1,{0x03}},
{0xA0,1,{0x0A}},
{0xA2,1,{0x03}},
{0xA3,1,{0x17}},
{0xA4,1,{0x03}},
{0xA5,1,{0x29}},
{0xA6,1,{0x03}},
{0xA7,1,{0x3D}},
{0xA9,1,{0x03}},
{0xAA,1,{0x52}},
{0xAB,1,{0x03}},
{0xAC,1,{0x6F}},
{0xAD,1,{0x03}},
{0xAE,1,{0x89}},
{0xAF,1,{0x03}},
{0xB0,1,{0xC8}},
{0xB1,1,{0x03}},
{0xB2,1,{0xFD}},
//R(-) MCR cm}},d
{0xB3,1,{0x00}},
{0xB4,1,{0x01}},
{0xB5,1,{0x00}},
{0xB6,1,{0x03}},
{0xB7,1,{0x00}},
{0xB8,1,{0x1D}},
{0xB9,1,{0x00}},
{0xBA,1,{0x36}},
{0xBB,1,{0x00}},
{0xBC,1,{0x49}},
{0xBD,1,{0x00}},
{0xBE,1,{0x5B}},
{0xBF,1,{0x00}},
{0xC0,1,{0x67}},
{0xC1,1,{0x00}},
{0xC2,1,{0x77}},
{0xC3,1,{0x00}},
{0xC4,1,{0x84}},
{0xC5,1,{0x00}},
{0xC6,1,{0xB3}},
{0xC7,1,{0x00}},
{0xC8,1,{0xD7}},
{0xC9,1,{0x01}},
{0xCA,1,{0x13}},
{0xCB,1,{0x01}},
{0xCC,1,{0x44}},
{0xCD,1,{0x01}},
{0xCE,1,{0x91}},
{0xCF,1,{0x01}},
{0xD0,1,{0xD1}},
{0xD1,1,{0x01}},
{0xD2,1,{0xD4}},
{0xD3,1,{0x02}},
{0xD4,1,{0x10}},
{0xD5,1,{0x02}},
{0xD6,1,{0x51}},
{0xD7,1,{0x02}},
{0xD8,1,{0x7C}},
{0xD9,1,{0x02}},
{0xDA,1,{0xB2}},
{0xDB,1,{0x02}},
{0xDC,1,{0xD7}},
{0xDD,1,{0x03}},
{0xDE,1,{0x0A}},
{0xDF,1,{0x03}},
{0xE0,1,{0x17}},
{0xE1,1,{0x03}},
{0xE2,1,{0x29}},
{0xE3,1,{0x03}},
{0xE4,1,{0x3D}},
{0xE5,1,{0x03}},
{0xE6,1,{0x52}},
{0xE7,1,{0x03}},
{0xE8,1,{0x6F}},
{0xE9,1,{0x03}},
{0xEA,1,{0x89}},
{0xEB,1,{0x03}},
{0xEC,1,{0xC8}},
{0xED,1,{0x03}},
{0xEE,1,{0xFD}},
//G(+) MCR c}},md
{0xEF,1,{0x00}},
{0xF0,1,{0x01}},
{0xF1,1,{0x00}},
{0xF2,1,{0x03}},
{0xF3,1,{0x00}},
{0xF4,1,{0x1F}},
{0xF5,1,{0x00}},
{0xF6,1,{0x36}},
{0xF7,1,{0x00}},
{0xF8,1,{0x4A}},
{0xF9,1,{0x00}},
{0xFA,1,{0x5C}},
//page selecti}},on cmd start
{0xFF,1,{0x21}},
{REGFLAG_DELAY, 2, {}},
{0xFB,1,{0x01}},

//page selecti}},on cmd end
{0x00,1,{0x00}},
{0x01,1,{0x6C}},
{0x02,1,{0x00}},
{0x03,1,{0x7A}},
{0x04,1,{0x00}},
{0x05,1,{0x87}},
{0x06,1,{0x00}},
{0x07,1,{0xB5}},
{0x08,1,{0x00}},
{0x09,1,{0xD9}},
{0x0A,1,{0x01}},
{0x0B,1,{0x15}},
{0x0C,1,{0x01}},
{0x0D,1,{0x46}},
{0x0E,1,{0x01}},
{0x0F,1,{0x93}},
{0x10,1,{0x01}},
{0x11,1,{0xD3}},
{0x12,1,{0x01}},
{0x13,1,{0xD5}},
{0x14,1,{0x02}},
{0x15,1,{0x10}},
{0x16,1,{0x02}},
{0x17,1,{0x52}},
{0x18,1,{0x02}},
{0x19,1,{0x7C}},
{0x1A,1,{0x02}},
{0x1B,1,{0xB3}},
{0x1C,1,{0x02}},
{0x1D,1,{0xD9}},
{0x1E,1,{0x03}},
{0x1F,1,{0x09}},
{0x20,1,{0x03}},
{0x21,1,{0x18}},
{0x22,1,{0x03}},
{0x23,1,{0x2A}},
{0x24,1,{0x03}},
{0x25,1,{0x3E}},
{0x26,1,{0x03}},
{0x27,1,{0x53}},
{0x28,1,{0x03}},
{0x29,1,{0x6F}},
{0x2A,1,{0x03}},
{0x2B,1,{0x90}},
{0x2D,1,{0x03}},
{0x2F,1,{0xBE}},
{0x30,1,{0x03}},
{0x31,1,{0xFD}},
//G(-) MCR c}},md
{0x32,1,{0x00}},
{0x33,1,{0x01}},
{0x34,1,{0x00}},
{0x35,1,{0x03}},
{0x36,1,{0x00}},
{0x37,1,{0x1F}},
{0x38,1,{0x00}},
{0x39,1,{0x36}},
{0x3A,1,{0x00}},
{0x3B,1,{0x4A}},
{0x3D,1,{0x00}},
{0x3F,1,{0x5C}},
{0x40,1,{0x00}},
{0x41,1,{0x6C}},
{0x42,1,{0x00}},
{0x43,1,{0x7A}},
{0x44,1,{0x00}},
{0x45,1,{0x87}},
{0x46,1,{0x00}},
{0x47,1,{0xB5}},
{0x48,1,{0x00}},
{0x49,1,{0xD9}},
{0x4A,1,{0x01}},
{0x4B,1,{0x15}},
{0x4C,1,{0x01}},
{0x4D,1,{0x46}},
{0x4E,1,{0x01}},
{0x4F,1,{0x93}},
{0x50,1,{0x01}},
{0x51,1,{0xD3}},
{0x52,1,{0x01}},
{0x53,1,{0xD5}},
{0x54,1,{0x02}},
{0x55,1,{0x10}},
{0x56,1,{0x02}},
{0x58,1,{0x52}},
{0x59,1,{0x02}},
{0x5A,1,{0x7C}},
{0x5B,1,{0x02}},
{0x5C,1,{0xB3}},
{0x5D,1,{0x02}},
{0x5E,1,{0xD9}},
{0x5F,1,{0x03}},
{0x60,1,{0x09}},
{0x61,1,{0x03}},
{0x62,1,{0x18}},
{0x63,1,{0x03}},
{0x64,1,{0x2A}},
{0x65,1,{0x03}},
{0x66,1,{0x3E}},
{0x67,1,{0x03}},
{0x68,1,{0x53}},
{0x69,1,{0x03}},
{0x6A,1,{0x6F}},
{0x6B,1,{0x03}},
{0x6C,1,{0x90}},
{0x6D,1,{0x03}},
{0x6E,1,{0xBE}},
{0x6F,1,{0x03}},
{0x70,1,{0xFD}},
//B(+) MCR c}},md
{0x71,1,{0x00}},
{0x72,1,{0x5D}},
{0x73,1,{0x00}},
{0x74,1,{0x5F}},
{0x75,1,{0x00}},
{0x76,1,{0x6D}},
{0x77,1,{0x00}},
{0x78,1,{0x79}},
{0x79,1,{0x00}},
{0x7A,1,{0x85}},
{0x7B,1,{0x00}},
{0x7C,1,{0x90}},
{0x7D,1,{0x00}},
{0x7E,1,{0x9B}},
{0x7F,1,{0x00}},
{0x80,1,{0xA5}},
{0x81,1,{0x00}},
{0x82,1,{0xAF}},
{0x83,1,{0x00}},
{0x84,1,{0xD2}},
{0x85,1,{0x00}},
{0x86,1,{0xF1}},
{0x87,1,{0x01}},
{0x88,1,{0x26}},
{0x89,1,{0x01}},
{0x8A,1,{0x51}},
{0x8B,1,{0x01}},
{0x8C,1,{0x9A}},
{0x8D,1,{0x01}},
{0x8E,1,{0xD8}},
{0x8F,1,{0x01}},
{0x90,1,{0xDA}},
{0x91,1,{0x02}},
{0x92,1,{0x14}},
{0x93,1,{0x02}},
{0x94,1,{0x56}},
{0x95,1,{0x02}},
{0x96,1,{0x80}},
{0x97,1,{0x02}},
{0x98,1,{0xBD}},
{0x99,1,{0x02}},
{0x9A,1,{0xE9}},
{0x9B,1,{0x03}},
{0x9C,1,{0x36}},
{0x9D,1,{0x03}},
{0x9E,1,{0x5E}},
{0x9F,1,{0x03}},
{0xA0,1,{0x6E}},
{0xA2,1,{0x03}},
{0xA3,1,{0x81}},
{0xA4,1,{0x03}},
{0xA5,1,{0x8F}},
{0xA6,1,{0x03}},
{0xA7,1,{0x95}},
{0xA9,1,{0x03}},
{0xAA,1,{0xDF}},
{0xAB,1,{0x03}},
{0xAC,1,{0xE1}},
{0xAD,1,{0x03}},
{0xAE,1,{0xFD}},
//B(-) MCR cm}},d
{0xAF,1,{0x00}},
{0xB0,1,{0x5D}},
{0xB1,1,{0x00}},
{0xB2,1,{0x5F}},
{0xB3,1,{0x00}},
{0xB4,1,{0x6D}},
{0xB5,1,{0x00}},
{0xB6,1,{0x79}},
{0xB7,1,{0x00}},
{0xB8,1,{0x85}},
{0xB9,1,{0x00}},
{0xBA,1,{0x90}},
{0xBB,1,{0x00}},
{0xBC,1,{0x9B}},
{0xBD,1,{0x00}},
{0xBE,1,{0xA5}},
{0xBF,1,{0x00}},
{0xC0,1,{0xAF}},
{0xC1,1,{0x00}},
{0xC2,1,{0xD2}},
{0xC3,1,{0x00}},
{0xC4,1,{0xF1}},
{0xC5,1,{0x01}},
{0xC6,1,{0x26}},
{0xC7,1,{0x01}},
{0xC8,1,{0x51}},
{0xC9,1,{0x01}},
{0xCA,1,{0x9A}},
{0xCB,1,{0x01}},
{0xCC,1,{0xD8}},
{0xCD,1,{0x01}},
{0xCE,1,{0xDA}},
{0xCF,1,{0x02}},
{0xD0,1,{0x14}},
{0xD1,1,{0x02}},
{0xD2,1,{0x56}},
{0xD3,1,{0x02}},
{0xD4,1,{0x80}},
{0xD5,1,{0x02}},
{0xD6,1,{0xBD}},
{0xD7,1,{0x02}},
{0xD8,1,{0xE9}},
{0xD9,1,{0x03}},
{0xDA,1,{0x36}},
{0xDB,1,{0x03}},
{0xDC,1,{0x5E}},
{0xDD,1,{0x03}},
{0xDE,1,{0x6E}},
{0xDF,1,{0x03}},
{0xE0,1,{0x81}},
{0xE1,1,{0x03}},
{0xE2,1,{0x8F}},
{0xE3,1,{0x03}},
{0xE4,1,{0x95}},
{0xE5,1,{0x03}},
{0xE6,1,{0xDF}},
{0xE7,1,{0x03}},
{0xE8,1,{0xE1}},
{0xE9,1,{0x03}},
{0xEA,1,{0xFD}},
	
	{0xFF,1,{0x10}},	
};

static struct LCM_setting_table lcm_gamma_low_color_setting[] = {	
};

static struct LCM_setting_table lcm_gamma_high_color_setting[] = {
};
#endif
#endif
/* lenovo-sw yexm1 add lcd effect function 20140314 end */

//update initial param for IC nt35520 0.01
static struct LCM_setting_table lcm_initialization_setting[] ={
       {0xFF,1,{0x10}},	//	Return	To	CMD1		
	{REGFLAG_DELAY, 2, {}},
	{0xFB,1,{0x01}},//
#if (LCM_DSI_CMD_MODE)
	{0xBB,1,{0x10}},//cmd mode
#else
	{0xBB,1,{0x03}}, //video mode
#endif
	{0x3B,5,{0x03,0x0A,0x0A,0x0A,0x0A}},	
	{0x51,1,{0xFF}},//lenovo-sw yexm1 modify,20140404 begin
	{0x53,1,{0x24}},						
	//{0x55,1,{0x02}},////lenovo-sw yexm1 modify,20140404 end						
	//{0x5E,1,{0x00}},						
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					
	//{0x11,0,{}},						
	//{REGFLAG_DELAY, 150, {}},
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	{0xFF,1,{0x24}},	//	CMD2	Page	4	Entrance	
	{REGFLAG_DELAY, 2, {}},
	{0xFB,1,{0x01}},	

	
	//{0xE3,1,{0x02}},      // LGD don't know this code.
	
	{0x9D,1,{0xB0}},						
	{0x72,1,{0x00}},						
	{0x93,1,{0x04}},						
	{0x94,1,{0x04}},						
	{0x9B,1,{0x0F}},						
	{0x8A,1,{0x33}},						
	{0x86,1,{0x1B}},						
	{0x87,1,{0x39}},						
	{0x88,1,{0x1B}},						
	{0x89,1,{0x39}},						
	{0x8B,1,{0xF4}},						
	{0x8C,1,{0x01}},						
	{0x90,1,{0x79}},						
	{0x91,1,{0x44}},
	{0x92,1,{0x79}},						
	{0x95,1,{0xE4}},	// This is for Ilde mode. This code don't exist in LGD code

	{0xDE,1,{0xFF}},	// This is for Ilde mode. This code don't exist in LGD code										
	{0xDF,1,{0x82}},	// This is for Ilde mode. This code don't exist in LGD code					

	{0x00,1,{0x0F}},						
	{0x01,1,{0x00}},						
	{0x02,1,{0x00}},						
	{0x03,1,{0x00}},						
	{0x04,1,{0x0B}},						
	{0x05,1,{0x0C}},						
	{0x06,1,{0x00}},						
	{0x07,1,{0x00}},						
	{0x08,1,{0x00}},						
	{0x09,1,{0x00}},						
	{0x0A,1,{0X03}},						
	{0x0B,1,{0X04}},						
	{0x0C,1,{0x01}},						
	{0x0D,1,{0x13}},						
	{0x0E,1,{0x15}},						
	{0x0F,1,{0x17}},						
	{0x10,1,{0x0F}},						
	{0x11,1,{0x00}},						
	{0x12,1,{0x00}},						
	{0x13,1,{0x00}},						
	{0x14,1,{0x0B}},						
	{0x15,1,{0x0C}},						
	{0x16,1,{0x00}},						
	{0x17,1,{0x00}},						
	{0x18,1,{0x00}},						
	{0x19,1,{0x00}},						
	{0x1A,1,{0x03}},						
	{0x1B,1,{0X04}},						
	{0x1C,1,{0x01}},						
	{0x1D,1,{0x13}},						
	{0x1E,1,{0x15}},						
	{0x1F,1,{0x17}},						

	{0x20,1,{0x09}},						
	{0x21,1,{0x01}},						
	{0x22,1,{0x00}},						
	{0x23,1,{0x00}},						
	{0x24,1,{0x00}},						
	{0x25,1,{0x6D}},						
	{0x26,1,{0x00}},						
	{0x27,1,{0x00}},						

	{0x2F,1,{0x02}},						
	{0x30,1,{0x04}},						
	{0x31,1,{0x49}},						
	{0x32,1,{0x23}},						
	{0x33,1,{0x01}},						
	{0x34,1,{0x00}},						
	{0x35,1,{0x69}},						
	{0x36,1,{0x00}},						
	{0x37,1,{0x2D}},						
	{0x38,1,{0x08}},						
	{0x39,1,{0x00}},						
	{0x3A,1,{0x69}},						

	{0x29,1,{0x58}},						
	{0x2A,1,{0x16}},						

	{0x5B,1,{0x00}},						
	{0x5F,1,{0x75}},						
	{0x63,1,{0x00}},						
	{0x67,1,{0x04}},						

	{0x7B,1,{0x80}},						
	{0x7C,1,{0xD8}},						
	{0x7D,1,{0x60}},						
	{0x7E,1,{0x0B}},	
	{0x7F,1,{0x17}},	
	{0x80,1,{0x00}},						
	{0x81,1,{0x06}},						
	{0x82,1,{0x03}},						
	{0x83,1,{0x00}},						
	{0x84,1,{0x03}},						
	{0x85,1,{0x07}},						
	{0x74,1,{0x0B}},	
	{0x75,1,{0x17}},	
	{0x76,1,{0x06}},						
	{0x77,1,{0x03}},						

	{0x78,1,{0x00}},						
	{0x79,1,{0x00}},						
	{0x99,1,{0x33}},						
	{0x98,1,{0x00}},						
	{0xB3,1,{0x28}},						
	{0xB4,1,{0x05}},						
	{0xB5,1,{0x10}},		

	{0xC4,1,{0x24}},	// This code don't exist in Lenovo code. LGD don't use this code. but please check Lenovo use or not					
	{0xC5,1,{0x30}},	// This code don't exist in Lenovo code. LGD don't use this code. but please check Lenovo use or not										
	{0xC6,1,{0x00}},	// This code don't exist in Lenovo code. LGD don't use this code. but please check Lenovo use or not										
		

	{0xFF,1,{0x20}},	//	Page	0,1,{	power-related	setting	
	{REGFLAG_DELAY, 2, {}},	
	{0xFB,1,{0x01}},//
	{0x00,1,{0x01}},						
	{0x01,1,{0x55}},						
	{0x02,1,{0x45}},						
	{0x03,1,{0x55}},						
	{0x05,1,{0x50}},						
	{0x06,1,{0x9E}},						
	{0x07,1,{0xA8}},						
	{0x08,1,{0x0C}},						
	{0x0B,1,{0x5F}},	
	{0x0C,1,{0x5F}},	
	{0x0E,1,{0xB5}},	
	{0x0F,1,{0xB8}},	
	//{0x11,1,{0x29}},	// Lenovo no need this code because LGD do MTP					
	//{0x12,1,{0x29}},	// Not exist code. please delete					
	{0x13,1,{0x03}},						
	{0x14,1,{0x0A}},						
	{0x15,1,{0x15}},	
	{0x16,1,{0x15}},	

	{0x18,1,{0x00}},	// This code don't exist in Lenovo code.

	{0x6D,1,{0x44}},						
	{0x58,1,{0x84}},						
	{0x59,1,{0x04}},						
	{0x5A,1,{0x04}},						
	{0x5B,1,{0x04}},						
	{0x5C,1,{0x82}},						
	{0x5D,1,{0x82}},						
	{0x5E,1,{0x02}},						
	{0x5F,1,{0x02}},						

	{0x1B,1,{0x39}},						
	{0x1C,1,{0x39}},						
	{0x1D,1,{0x47}},						
		//page selection cmd start


//page selection cmd start
{0xFF,1,{0x20}},
{REGFLAG_DELAY, 2, {}},
{0xFB,1,{0x01}},

//page selecti}},on cmd end
//R(+) MCR c}},md
{0x75,1,{0x00}},
{0x76,1,{0x01}},
{0x77,1,{0x00}},
{0x78,1,{0x03}},
{0x79,1,{0x00}},
{0x7A,1,{0x1D}},
{0x7B,1,{0x00}},
{0x7C,1,{0x36}},
{0x7D,1,{0x00}},
{0x7E,1,{0x49}},
{0x7F,1,{0x00}},
{0x80,1,{0x5B}},
{0x81,1,{0x00}},
{0x82,1,{0x67}},
{0x83,1,{0x00}},
{0x84,1,{0x77}},
{0x85,1,{0x00}},
{0x86,1,{0x84}},
{0x87,1,{0x00}},
{0x88,1,{0xB3}},
{0x89,1,{0x00}},
{0x8A,1,{0xD7}},
{0x8B,1,{0x01}},
{0x8C,1,{0x13}},
{0x8D,1,{0x01}},
{0x8E,1,{0x44}},
{0x8F,1,{0x01}},
{0x90,1,{0x91}},
{0x91,1,{0x01}},
{0x92,1,{0xD1}},
{0x93,1,{0x01}},
{0x94,1,{0xD4}},
{0x95,1,{0x02}},
{0x96,1,{0x10}},
{0x97,1,{0x02}},
{0x98,1,{0x51}},
{0x99,1,{0x02}},
{0x9A,1,{0x7C}},
{0x9B,1,{0x02}},
{0x9C,1,{0xB2}},
{0x9D,1,{0x02}},
{0x9E,1,{0xD7}},
{0x9F,1,{0x03}},
{0xA0,1,{0x0A}},
{0xA2,1,{0x03}},
{0xA3,1,{0x17}},
{0xA4,1,{0x03}},
{0xA5,1,{0x29}},
{0xA6,1,{0x03}},
{0xA7,1,{0x3D}},
{0xA9,1,{0x03}},
{0xAA,1,{0x52}},
{0xAB,1,{0x03}},
{0xAC,1,{0x6F}},
{0xAD,1,{0x03}},
{0xAE,1,{0x89}},
{0xAF,1,{0x03}},
{0xB0,1,{0xC8}},
{0xB1,1,{0x03}},
{0xB2,1,{0xFD}},
//R(-) MCR cm}},d
{0xB3,1,{0x00}},
{0xB4,1,{0x01}},
{0xB5,1,{0x00}},
{0xB6,1,{0x03}},
{0xB7,1,{0x00}},
{0xB8,1,{0x1D}},
{0xB9,1,{0x00}},
{0xBA,1,{0x36}},
{0xBB,1,{0x00}},
{0xBC,1,{0x49}},
{0xBD,1,{0x00}},
{0xBE,1,{0x5B}},
{0xBF,1,{0x00}},
{0xC0,1,{0x67}},
{0xC1,1,{0x00}},
{0xC2,1,{0x77}},
{0xC3,1,{0x00}},
{0xC4,1,{0x84}},
{0xC5,1,{0x00}},
{0xC6,1,{0xB3}},
{0xC7,1,{0x00}},
{0xC8,1,{0xD7}},
{0xC9,1,{0x01}},
{0xCA,1,{0x13}},
{0xCB,1,{0x01}},
{0xCC,1,{0x44}},
{0xCD,1,{0x01}},
{0xCE,1,{0x91}},
{0xCF,1,{0x01}},
{0xD0,1,{0xD1}},
{0xD1,1,{0x01}},
{0xD2,1,{0xD4}},
{0xD3,1,{0x02}},
{0xD4,1,{0x10}},
{0xD5,1,{0x02}},
{0xD6,1,{0x51}},
{0xD7,1,{0x02}},
{0xD8,1,{0x7C}},
{0xD9,1,{0x02}},
{0xDA,1,{0xB2}},
{0xDB,1,{0x02}},
{0xDC,1,{0xD7}},
{0xDD,1,{0x03}},
{0xDE,1,{0x0A}},
{0xDF,1,{0x03}},
{0xE0,1,{0x17}},
{0xE1,1,{0x03}},
{0xE2,1,{0x29}},
{0xE3,1,{0x03}},
{0xE4,1,{0x3D}},
{0xE5,1,{0x03}},
{0xE6,1,{0x52}},
{0xE7,1,{0x03}},
{0xE8,1,{0x6F}},
{0xE9,1,{0x03}},
{0xEA,1,{0x89}},
{0xEB,1,{0x03}},
{0xEC,1,{0xC8}},
{0xED,1,{0x03}},
{0xEE,1,{0xFD}},
//G(+) MCR c}},md
{0xEF,1,{0x00}},
{0xF0,1,{0x01}},
{0xF1,1,{0x00}},
{0xF2,1,{0x03}},
{0xF3,1,{0x00}},
{0xF4,1,{0x1F}},
{0xF5,1,{0x00}},
{0xF6,1,{0x36}},
{0xF7,1,{0x00}},
{0xF8,1,{0x4A}},
{0xF9,1,{0x00}},
{0xFA,1,{0x5C}},
//page selecti}},on cmd start
{0xFF,1,{0x21}},
{REGFLAG_DELAY, 2, {}},
{0xFB,1,{0x01}},

//page selecti}},on cmd end
{0x00,1,{0x00}},
{0x01,1,{0x6C}},
{0x02,1,{0x00}},
{0x03,1,{0x7A}},
{0x04,1,{0x00}},
{0x05,1,{0x87}},
{0x06,1,{0x00}},
{0x07,1,{0xB5}},
{0x08,1,{0x00}},
{0x09,1,{0xD9}},
{0x0A,1,{0x01}},
{0x0B,1,{0x15}},
{0x0C,1,{0x01}},
{0x0D,1,{0x46}},
{0x0E,1,{0x01}},
{0x0F,1,{0x93}},
{0x10,1,{0x01}},
{0x11,1,{0xD3}},
{0x12,1,{0x01}},
{0x13,1,{0xD5}},
{0x14,1,{0x02}},
{0x15,1,{0x10}},
{0x16,1,{0x02}},
{0x17,1,{0x52}},
{0x18,1,{0x02}},
{0x19,1,{0x7C}},
{0x1A,1,{0x02}},
{0x1B,1,{0xB3}},
{0x1C,1,{0x02}},
{0x1D,1,{0xD9}},
{0x1E,1,{0x03}},
{0x1F,1,{0x09}},
{0x20,1,{0x03}},
{0x21,1,{0x18}},
{0x22,1,{0x03}},
{0x23,1,{0x2A}},
{0x24,1,{0x03}},
{0x25,1,{0x3E}},
{0x26,1,{0x03}},
{0x27,1,{0x53}},
{0x28,1,{0x03}},
{0x29,1,{0x6F}},
{0x2A,1,{0x03}},
{0x2B,1,{0x90}},
{0x2D,1,{0x03}},
{0x2F,1,{0xBE}},
{0x30,1,{0x03}},
{0x31,1,{0xFD}},
//G(-) MCR c}},md
{0x32,1,{0x00}},
{0x33,1,{0x01}},
{0x34,1,{0x00}},
{0x35,1,{0x03}},
{0x36,1,{0x00}},
{0x37,1,{0x1F}},
{0x38,1,{0x00}},
{0x39,1,{0x36}},
{0x3A,1,{0x00}},
{0x3B,1,{0x4A}},
{0x3D,1,{0x00}},
{0x3F,1,{0x5C}},
{0x40,1,{0x00}},
{0x41,1,{0x6C}},
{0x42,1,{0x00}},
{0x43,1,{0x7A}},
{0x44,1,{0x00}},
{0x45,1,{0x87}},
{0x46,1,{0x00}},
{0x47,1,{0xB5}},
{0x48,1,{0x00}},
{0x49,1,{0xD9}},
{0x4A,1,{0x01}},
{0x4B,1,{0x15}},
{0x4C,1,{0x01}},
{0x4D,1,{0x46}},
{0x4E,1,{0x01}},
{0x4F,1,{0x93}},
{0x50,1,{0x01}},
{0x51,1,{0xD3}},
{0x52,1,{0x01}},
{0x53,1,{0xD5}},
{0x54,1,{0x02}},
{0x55,1,{0x10}},
{0x56,1,{0x02}},
{0x58,1,{0x52}},
{0x59,1,{0x02}},
{0x5A,1,{0x7C}},
{0x5B,1,{0x02}},
{0x5C,1,{0xB3}},
{0x5D,1,{0x02}},
{0x5E,1,{0xD9}},
{0x5F,1,{0x03}},
{0x60,1,{0x09}},
{0x61,1,{0x03}},
{0x62,1,{0x18}},
{0x63,1,{0x03}},
{0x64,1,{0x2A}},
{0x65,1,{0x03}},
{0x66,1,{0x3E}},
{0x67,1,{0x03}},
{0x68,1,{0x53}},
{0x69,1,{0x03}},
{0x6A,1,{0x6F}},
{0x6B,1,{0x03}},
{0x6C,1,{0x90}},
{0x6D,1,{0x03}},
{0x6E,1,{0xBE}},
{0x6F,1,{0x03}},
{0x70,1,{0xFD}},
//B(+) MCR c}},md
{0x71,1,{0x00}},
{0x72,1,{0x5D}},
{0x73,1,{0x00}},
{0x74,1,{0x5F}},
{0x75,1,{0x00}},
{0x76,1,{0x6D}},
{0x77,1,{0x00}},
{0x78,1,{0x79}},
{0x79,1,{0x00}},
{0x7A,1,{0x85}},
{0x7B,1,{0x00}},
{0x7C,1,{0x90}},
{0x7D,1,{0x00}},
{0x7E,1,{0x9B}},
{0x7F,1,{0x00}},
{0x80,1,{0xA5}},
{0x81,1,{0x00}},
{0x82,1,{0xAF}},
{0x83,1,{0x00}},
{0x84,1,{0xD2}},
{0x85,1,{0x00}},
{0x86,1,{0xF1}},
{0x87,1,{0x01}},
{0x88,1,{0x26}},
{0x89,1,{0x01}},
{0x8A,1,{0x51}},
{0x8B,1,{0x01}},
{0x8C,1,{0x9A}},
{0x8D,1,{0x01}},
{0x8E,1,{0xD8}},
{0x8F,1,{0x01}},
{0x90,1,{0xDA}},
{0x91,1,{0x02}},
{0x92,1,{0x14}},
{0x93,1,{0x02}},
{0x94,1,{0x56}},
{0x95,1,{0x02}},
{0x96,1,{0x80}},
{0x97,1,{0x02}},
{0x98,1,{0xBD}},
{0x99,1,{0x02}},
{0x9A,1,{0xE9}},
{0x9B,1,{0x03}},
{0x9C,1,{0x36}},
{0x9D,1,{0x03}},
{0x9E,1,{0x5E}},
{0x9F,1,{0x03}},
{0xA0,1,{0x6E}},
{0xA2,1,{0x03}},
{0xA3,1,{0x81}},
{0xA4,1,{0x03}},
{0xA5,1,{0x8F}},
{0xA6,1,{0x03}},
{0xA7,1,{0x95}},
{0xA9,1,{0x03}},
{0xAA,1,{0xDF}},
{0xAB,1,{0x03}},
{0xAC,1,{0xE1}},
{0xAD,1,{0x03}},
{0xAE,1,{0xFD}},
//B(-) MCR cm}},d
{0xAF,1,{0x00}},
{0xB0,1,{0x5D}},
{0xB1,1,{0x00}},
{0xB2,1,{0x5F}},
{0xB3,1,{0x00}},
{0xB4,1,{0x6D}},
{0xB5,1,{0x00}},
{0xB6,1,{0x79}},
{0xB7,1,{0x00}},
{0xB8,1,{0x85}},
{0xB9,1,{0x00}},
{0xBA,1,{0x90}},
{0xBB,1,{0x00}},
{0xBC,1,{0x9B}},
{0xBD,1,{0x00}},
{0xBE,1,{0xA5}},
{0xBF,1,{0x00}},
{0xC0,1,{0xAF}},
{0xC1,1,{0x00}},
{0xC2,1,{0xD2}},
{0xC3,1,{0x00}},
{0xC4,1,{0xF1}},
{0xC5,1,{0x01}},
{0xC6,1,{0x26}},
{0xC7,1,{0x01}},
{0xC8,1,{0x51}},
{0xC9,1,{0x01}},
{0xCA,1,{0x9A}},
{0xCB,1,{0x01}},
{0xCC,1,{0xD8}},
{0xCD,1,{0x01}},
{0xCE,1,{0xDA}},
{0xCF,1,{0x02}},
{0xD0,1,{0x14}},
{0xD1,1,{0x02}},
{0xD2,1,{0x56}},
{0xD3,1,{0x02}},
{0xD4,1,{0x80}},
{0xD5,1,{0x02}},
{0xD6,1,{0xBD}},
{0xD7,1,{0x02}},
{0xD8,1,{0xE9}},
{0xD9,1,{0x03}},
{0xDA,1,{0x36}},
{0xDB,1,{0x03}},
{0xDC,1,{0x5E}},
{0xDD,1,{0x03}},
{0xDE,1,{0x6E}},
{0xDF,1,{0x03}},
{0xE0,1,{0x81}},
{0xE1,1,{0x03}},
{0xE2,1,{0x8F}},
{0xE3,1,{0x03}},
{0xE4,1,{0x95}},
{0xE5,1,{0x03}},
{0xE6,1,{0xDF}},
{0xE7,1,{0x03}},
{0xE8,1,{0xE1}},
{0xE9,1,{0x03}},
{0xEA,1,{0xFD}},




	//{0xFF,1,{0x24}},	//	CMD2	Page	3	Entrance	
	//{REGFLAG_DELAY, 2, {}},	
	//{0xC4,1,{0x60}},//
	//{0xC9,1,{0x00}},		//10.5Mhz
	//{0xFB,1,{0x01}},		
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//image.first
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	{0xFF,1,{0x10}},	//	Return	To	CMD1		
	{REGFLAG_DELAY, 2, {}},				
	{0xFB,1,{0x01}},//
	
	{0x35,1,{0x00}},	
#if 1
	{0x11, 1, {0x00}},
	{REGFLAG_DELAY, 200, {}},

       //Display ON
       {0x29, 1, {0x00}},
       {REGFLAG_DELAY, 50, {}},
       {REGFLAG_END_OF_TABLE, 0x00, {}}

#else
	{0xFF,1,{0x24}},
	{REGFLAG_DELAY, 2, {}},	

	{0xEA,1,{0x01}},
	{0xEB,1,{0x20}},	
	{0xEC,1,{0x01}},
#endif
};
							
#if 0
static struct LCM_setting_table lcm_set_window[] = {
	{0x2A,	4,	{0x00, 0x00, (FRAME_WIDTH>>8), (FRAME_WIDTH&0xFF)}},
	{0x2B,	4,	{0x00, 0x00, (FRAME_HEIGHT>>8), (FRAME_HEIGHT&0xFF)}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
#endif
#if 1
static struct LCM_setting_table lcm_sleep_out_setting[] = {
    //Sleep Out
    {0x11, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
    {0x29, 1, {0x00}},
    {REGFLAG_DELAY, 20, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};
#endif

static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
    // Display off sequence
    {0x28, 1, {0x00}},
    {REGFLAG_DELAY, 20, {}},

    // Sleep Mode On
    {0x10, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},
    //{0x4F, 1, {0x01}},//ENTER DEEP SLEEP MODE
    //{REGFLAG_DELAY, 120, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void push_table2(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;

    for(i = 0; i < count; i++)
    {
        unsigned cmd;
        cmd = table[i].cmd;

        switch (cmd) {

            case REGFLAG_DELAY :
                if(table[i].count <= 10)
                    MDELAY(table[i].count);
                else
                    MDELAY(table[i].count);
                break;

            case REGFLAG_END_OF_TABLE :
                break;

            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
        }
    }
}

static void push_table(void* handle,struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;

    for(i = 0; i < count; i++)
    {
        unsigned cmd;
        cmd = table[i].cmd;

        switch (cmd) {

            case REGFLAG_DELAY :
                if(table[i].count <= 10)
                    MDELAY(table[i].count);
                else
                    MDELAY(table[i].count);
                break;

            case REGFLAG_END_OF_TABLE :
                break;

            default:
                dsi_set_cmd_by_cmdq(handle,cmd, table[i].count, table[i].para_list, force_update);
        }
    }
}
// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type   = LCM_TYPE_DSI;

	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
	params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
#else
    params->dsi.mode   = SYNC_PULSE_VDO_MODE;
	params->dsi.switch_mode = CMD_MODE;
#endif
	params->dsi.switch_mode_enable = 1;

	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM				= LCM_FOUR_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order 	= LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   	= LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     	= LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      		= LCM_DSI_FORMAT_RGB888;

	// Highly depends on LCD driver capability.
	params->dsi.packet_size=256;
	//video mode timing
//	params->dsi.esd_check_enable=1;
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active				= 2;
	params->dsi.vertical_backporch					= 8;
	params->dsi.vertical_frontporch					= 10;
	params->dsi.vertical_active_line					= FRAME_HEIGHT;

	params->dsi.horizontal_sync_active				= 10;
	params->dsi.horizontal_backporch				= 20;
	params->dsi.horizontal_frontporch				= 40;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	//begin:haobing modified
	/*BEGIN PN:DTS2013013101431 modified by s00179437 , 2013-01-31*/
	//improve clk quality
	params->dsi.PLL_CLOCK = 500; //this value must be in MTK suggested table
	//params->dsi.pll_div1=0;		// div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
	//params->dsi.pll_div2=1;		// div2=0,1,2,3;div1_real=1,2,4,4	
	//params->dsi.fbk_div =21;    	// fref=26MHz, fvco=fref*(fbk_div)*2/(div1_real*div2_real)	
	/*END PN:DTS2013013101431 modified by s00179437 , 2013-01-31*/
	//end:haobing modified

	//lenovo-sw wangyq13 add for esd check start 20140606
	params->dsi.cont_clock = 0;
	params->dsi.clk_lp_per_line_enable = 1;
	params->dsi.esd_check_enable = 1;//  1
	params->dsi.customization_esd_check_enable = 0;  //   1
	params->dsi.lcm_esd_check_table[0].cmd          = 0x0A; //0xAB
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
	//params->dsi.lcm_esd_check_table[0].para_list[1] = 0x00;
	//lenovo-sw wangyq13 add for esd check end 20140606

}


#if 0
static void lcm_esd_read(void)
{
	unsigned char buffer[2];
	unsigned int array[16];

	array[0]=0x00023902;
	array[1]=0x000010FF;
	dsi_set_cmdq(array, 2, 1);
	MDELAY(2);

	array[0]=0x00023902;
	array[1]=0x000001FB;
	dsi_set_cmdq(array, 2, 1);
	MDELAY(2);

	array[0] = 0x00023700;	// read id return two byte,
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x0A, buffer, 1);
#ifdef BUILD_LK
	printf(" LK nt35595 ESD_READ  : 0x0a_byte0=0x%x\n",buffer[0]);
#else
	printk(" LK nt35595 ESD_READ  : 0x0a_byte0=0x%x\n",buffer[0]);
	        
#endif

}



static void lcm_esd_read_BB(void)
{
	unsigned char buffer[2];
	unsigned int array[16];


	array[0]=0x00023902;
	array[1]=0x000010FF;
	dsi_set_cmdq(array, 2, 1);
	MDELAY(2);

	array[0]=0x00023902;
	array[1]=0x000001FB;
	dsi_set_cmdq(array, 2, 1);
	MDELAY(2);

	array[0] = 0x00023700;	// read id return two byte,
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xBB, buffer, 1);
#ifdef BUILD_LK
	printf(" LK nt35595 ESD_READ  : 0xbb_byte0=0x%x\n",buffer[0]);
#else
	printk(" LK nt35595 ESD_READ  : 0xbb_byte0=0x%x\n",buffer[0]);
	        
#endif
}
#endif

/* Lenovo-sw yexm1 add 20140402 begin */
//#ifdef LENOVO_LCD_BACKLIGHT_CONTROL_BY_LCM
//static unsigned int bl_level = 0;
static void lcm_setbacklight(unsigned int level)
{
#if 1
	unsigned char data_array[16];
	//if(isAAL == 1) return;
	if(level > 255) 
	level = 255;

	#ifndef BUILD_LK
	printk("[yexm1] %s level is %d\n",__func__,level);
	#endif
	data_array[0] = level;       
	dsi_set_cmdq_V2(0x51, 1, data_array, 1); 
	//bl_level = level;
#endif
}
//#endif
/* Lenovo-sw yexm1 add 20140402 end */

static void lcm_init(void)
{
//#ifdef BUILD_LK
#if 0
	if(isAAL == 3)
	{
		mt_set_gpio_mode(GPIO_AAL_ID, GPIO_MODE_00);
    		mt_set_gpio_dir(GPIO_AAL_ID, GPIO_DIR_IN);
		isAAL = mt_get_gpio_in(GPIO_AAL_ID);
		dprintf(0, "[lenovo] %s isAAL is %d \n",__func__,isAAL);
	}
#endif
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
    	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
    	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);
	MDELAY(10);
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_MODE_00);
    	mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);
    	mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ONE);



       MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(20);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(20);
	//lcm_esd_read();


		// when phone initial , config output high, enable backlight drv chip  
	push_table2(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);  

	#ifndef BUILD_LK
	//#ifdef LENOVO_LCD_BACKLIGHT_CONTROL_BY_LCM
	lcm_setbacklight(255);
	//#endif
	//printk("ESD_READ FAIL_LCM_INIT[yexm1] s%\n",__func__);
	#endif
	//lcm_esd_read();
	//lcm_esd_read_BB();

}


static void lcm_suspend(void)
{
 	push_table2(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1); 
 	mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_MODE_00);
    	mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);
    	mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ZERO);
	MDELAY(6);
 	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
 	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
 	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ZERO);

  /*lenovo-sw louhs1, for low power,  20140801 begin*/
    //close the bl river ic enable
    mt_set_gpio_out(GPIO_LCM_BL_EN, GPIO_OUT_ZERO);
/*lenovo-sw louhs1, for low power,  20140801 end*/
}
static void lcm_resume(void)
{
  /*lenovo-sw louhs1, for low power,  20140801 begin*/
    //open the bl river ic enable
    mt_set_gpio_out(GPIO_LCM_BL_EN, GPIO_OUT_ONE);
/*lenovo-sw louhs1, for low power,  20140801 end*/

	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
 	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
 	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);
	MDELAY(6);
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_MODE_00);
    	mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);
    	mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ONE);
	MDELAY(20);

	/*SET_RESET_PIN(1);
    	SET_RESET_PIN(0);
    	MDELAY(10);
    	SET_RESET_PIN(1);
    	MDELAY(10);
	lcm_init(); */
 
  	push_table2(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1); 
}
static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	
	data_array[0]= 0x00053902;
	data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[2]= (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
         /*BEGIN PN:DTS2013013101431 modified by s00179437 , 2013-01-31*/
         //delete high speed packet
	//data_array[0]=0x00290508;
	//dsi_set_cmdq(data_array, 1, 1);
         /*END PN:DTS2013013101431 modified by s00179437 , 2013-01-31*/
	
	data_array[0]= 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);	
}


/*lenovo-sw zhouyj5 add LCD automatic recognition mechanism. 20140521 start */
#define LENOVO_LCD_ID_PIN (GPIO171 | 0x80000000)
static unsigned int lcm_compare_id(void)
{
	unsigned  int ret = 0;
	ret = mt_get_gpio_in(LENOVO_LCD_ID_PIN);
#ifdef BUILD_LK
	if(1 == ret)
		dprintf(0, "[LK]lg_nt35595 found\n");
#else
	if(1 == ret)
		printk("[KERNEL]lg_nt35595 found\n");
#endif
	return (1 == ret) ? 1: 0;
}
/*lenovo-sw zhouyj5 add LCD automatic recognition mechanism. 20140521 end */	

/* lenovo-sw yexm1 add lcd effect function 20140314 start */
#ifndef BUILD_LK
#ifdef CONFIG_LENOVO_LCM_EFFECT
static void lcm_setgamma_default(handle)
{
push_table(handle,lcm_gamma_default_setting, sizeof(lcm_gamma_default_setting) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_setgamma_cosy(handle)
{
push_table(handle,lcm_gamma_cosy_setting, sizeof(lcm_gamma_cosy_setting) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_setgamma_sunshine(handle)
{
push_table(handle,lcm_gamma_sunshine_setting, sizeof(lcm_gamma_sunshine_setting) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_setgamma_low_color(handle)
{
push_table(handle,lcm_gamma_low_color_setting, sizeof(lcm_gamma_low_color_setting) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_setgamma_high_color(handle)
{
push_table(handle,lcm_gamma_high_color_setting, sizeof(lcm_gamma_high_color_setting) / sizeof(struct LCM_setting_table), 1); 
}

// for CT start more cold
static void lcm_set_CT_0291_0306(handle) 
{
push_table(handle,lcm_CT_0291_0306, sizeof(lcm_CT_0291_0306) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CT_0292_0307(handle) 
{
push_table(handle,lcm_CT_0292_0307, sizeof(lcm_CT_0292_0307) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CT_0293_0308(handle) 
{
push_table(handle,lcm_CT_0293_0308, sizeof(lcm_CT_0293_0308) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CT_0294_0309(handle) 
{
push_table(handle,lcm_CT_0294_0309, sizeof(lcm_CT_0294_0309) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CT_0295_0310(handle) 
{
push_table(handle,lcm_CT_0295_0310, sizeof(lcm_CT_0295_0310) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CT_0296_0311(handle) 
{
push_table(handle,lcm_CT_0296_0311, sizeof(lcm_CT_0296_0311) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CT_0297_0312(handle) 
{
push_table(handle,lcm_CT_0297_0312, sizeof(lcm_CT_0297_0312) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CT_0298_0314(handle) 
{
push_table(handle,lcm_CT_0298_0314, sizeof(lcm_CT_0298_0314) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CT_0299_0315(handle) 
{
push_table(handle,lcm_CT_0299_0315, sizeof(lcm_CT_0299_0315) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CT_0300_0316(handle) 
{
push_table(handle,lcm_CT_0300_0316, sizeof(lcm_CT_0300_0316) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CT_0300_0320(handle) 
{
push_table(handle,lcm_CT_0300_0320, sizeof(lcm_CT_0300_0320) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CT_0302_0318(handle) 
{
push_table(handle,lcm_CT_0302_0318, sizeof(lcm_CT_0302_0318) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CT_0304_0320(handle) 
{
push_table(handle,lcm_CT_0304_0320, sizeof(lcm_CT_0304_0320) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CT_0305_0322(handle) 
{
push_table(handle,lcm_CT_0305_0322, sizeof(lcm_CT_0305_0322) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CT_0307_0323(handle) 
{
push_table(handle,lcm_CT_0307_0323, sizeof(lcm_CT_0307_0323) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CT_0308_0324(handle) 
{
push_table(handle,lcm_CT_0308_0324, sizeof(lcm_CT_0308_0324) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CT_0309_0326(handle) 
{
push_table(handle,lcm_CT_0309_0326, sizeof(lcm_CT_0309_0326) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CT_0311_0327(handle) 
{
push_table(handle,lcm_CT_0311_0327, sizeof(lcm_CT_0311_0327) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CT_0313_0329(handle) 
{
push_table(handle,lcm_CT_0313_0329, sizeof(lcm_CT_0313_0329) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CT_0314_0331(handle) 
{
push_table(handle,lcm_CT_0314_0331, sizeof(lcm_CT_0314_0331) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CT_0316_0332(handle) 
{
push_table(handle,lcm_CT_0316_0332, sizeof(lcm_CT_0316_0332) / sizeof(struct LCM_setting_table), 1); 
}

// for CT end more warm

//CE start level 1
static void lcm_set_CE_1(handle) 
{
push_table(handle,lcm_CE_1, sizeof(lcm_CE_1) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CE_2(handle) 
{
push_table(handle,lcm_CE_2, sizeof(lcm_CE_2) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CE_3(handle) 
{
push_table(handle,lcm_CE_3, sizeof(lcm_CE_3) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CE_4(handle) 
{
push_table(handle,lcm_CE_4, sizeof(lcm_CE_4) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CE_5(handle) 
{
push_table(handle,lcm_CE_5, sizeof(lcm_CE_5) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CE_6(handle) 
{
push_table(handle,lcm_CE_6, sizeof(lcm_CE_6) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CE_7(handle) 
{
push_table(handle,lcm_CE_7, sizeof(lcm_CE_7) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CE_8(handle) 
{
push_table(handle,lcm_CE_8, sizeof(lcm_CE_8) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CE_9(handle) 
{
push_table(handle,lcm_CE_9, sizeof(lcm_CE_9) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_CE_10(handle) 
{
push_table(handle,lcm_CE_10, sizeof(lcm_CE_10) / sizeof(struct LCM_setting_table), 1); 
}
//CE end level 10

//SC start level 1
static void lcm_set_SC_1(handle) 
{
push_table(handle,lcm_SC_1, sizeof(lcm_SC_1) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_SC_2(handle) 
{
push_table(handle,lcm_SC_2, sizeof(lcm_SC_2) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_SC_3(handle) 
{
push_table(handle,lcm_SC_3, sizeof(lcm_SC_3) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_SC_4(handle) 
{
push_table(handle,lcm_SC_4, sizeof(lcm_SC_4) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_SC_5(handle) 
{
push_table(handle,lcm_SC_5, sizeof(lcm_SC_5) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_SC_6(handle) 
{
push_table(handle,lcm_SC_6, sizeof(lcm_SC_6) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_SC_7(handle) 
{
push_table(handle,lcm_SC_7, sizeof(lcm_SC_7) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_SC_8(handle) 
{
push_table(handle,lcm_SC_8, sizeof(lcm_SC_8) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_SC_9(handle) 
{
push_table(handle,lcm_SC_9, sizeof(lcm_SC_9) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_set_SC_10(handle) 
{
push_table(handle,lcm_SC_10, sizeof(lcm_SC_10) / sizeof(struct LCM_setting_table), 1); 
}
//SC end level 10

static void lcm_setgammamode(void* handle,unsigned int mode)
{

	unsigned int data_array[16];
	#if BUILD_LK
	printf("%s mode=%d\n",__func__,mode);
	#else
	printk("%s mode=%d\n",__func__,mode);
	#endif

	 if(lcm_gammamode_index == mode)
		return;
	lcm_gammamode_index = mode;

	switch(mode){
		case 0:
			lcm_setgamma_default(handle);
			break;
		case 1:
			lcm_setgamma_cosy(handle);
			break;
		case 2:
			lcm_setgamma_sunshine(handle);
			break;
		case 3:
			lcm_setgamma_low_color(handle);
			break;
		case 4:
			lcm_setgamma_high_color(handle);
			break;
		//CT start
		case 5:
			lcm_set_CT_0291_0306(handle);
			break;
		case 6:
			lcm_set_CT_0292_0307(handle);
			break;
		case 7:
			lcm_set_CT_0293_0308(handle);
			break;
		case 8:
			lcm_set_CT_0294_0309(handle);
			break;
		case 9:
			lcm_set_CT_0295_0310(handle);
			break;
		case 10:
			lcm_set_CT_0296_0311(handle);
			break;
		case 11:
			lcm_set_CT_0297_0312(handle);
			break;
		case 12:
			lcm_set_CT_0298_0314(handle);
			break;
		case 13:
			lcm_set_CT_0299_0315(handle);
			break;
		case 14:
			lcm_set_CT_0300_0316(handle);
			break;
		case 15:
			lcm_set_CT_0300_0320(handle);
			break;
		case 16:
			lcm_set_CT_0302_0318(handle);
			break;
		case 17:
			lcm_set_CT_0304_0320(handle);
			break;
		case 18:
			lcm_set_CT_0305_0322(handle);
			break;
		case 19:
			lcm_set_CT_0307_0323(handle);
			break;
		case 20:
			lcm_set_CT_0308_0324(handle);
			break;
		case 21:
			lcm_set_CT_0309_0326(handle);
			break;
		case 22:
			lcm_set_CT_0311_0327(handle);
			break;
		case 23:
			lcm_set_CT_0313_0329(handle);
			break;
		case 24:
			lcm_set_CT_0314_0331(handle);
			break;
		case 25:
			lcm_set_CT_0316_0332(handle);
			break;
		//CE start
		case 26:
			lcm_set_CE_1(handle);
			break;
		case 27:
			lcm_set_CE_2(handle);
			break;
		case 28:
			lcm_set_CE_3(handle);
			break;
		case 29:
			lcm_set_CE_4(handle);
			break;
		case 30:
			lcm_set_CE_5(handle);
			break;
		case 31:
			lcm_set_CE_6(handle);
			break;
		case 32:
			lcm_set_CE_7(handle);
			break;
		case 33:
			lcm_set_CE_8(handle);
			break;
		case 34:
			lcm_set_CE_9(handle);
			break;
		case 35:
			lcm_set_CE_10(handle);
			break;
		//SC start
		case 36:
			lcm_set_SC_1(handle);
			break;
		case 37:
			lcm_set_SC_2(handle);
			break;
		case 38:
			lcm_set_SC_3(handle);
			break;
		case 39:
			lcm_set_SC_4(handle);
			break;
		case 40:
			lcm_set_SC_5(handle);
			break;
		case 41:
			lcm_set_SC_6(handle);
			break;
		case 42:
			lcm_set_SC_7(handle);
			break;
		case 43:
			lcm_set_SC_8(handle);
			break;
		case 44:
			lcm_set_SC_9(handle);
			break;
		case 45:
			lcm_set_SC_10(handle);
			break;
		default:
			break;
	}
//MDELAY(10);
}


static void lcm_setinversemode(void* handle,unsigned int mode)
{
	#ifndef BUILD_LK
	unsigned int data_array[16];	
	printk("%s on=%d\n",__func__,mode);
	switch(mode){
		case 0:
			data_array[0]=0x00; 
			dsi_set_cmd_by_cmdq(handle,0x20, 1, data_array, 1); 
			//lcm_inversemode_index = 0;
			break;
		case 1:
			data_array[0]=0x00; 
			dsi_set_cmd_by_cmdq(handle,0x21, 1, data_array, 1); 
			//lcm_inversemode_index = 1;
			break;
		default:
			break;
		}

	 MDELAY(10);
	 #endif
}

static void lcm_setcabcmode(void* handle,unsigned int mode)
{
#if 0
#ifndef BUILD_LK
	unsigned char buffer[8] = {0x0};
       //unsigned int arry[4] ;
       //arry[0] =0x00043700;
	//dsi_set_cmdq(arry, 1, 4);
	//read_reg_v2(0x56, buffer,1);
	//printk("setcabcmode begin = 0x%x,mode=%d\n",buffer[0],mode);
	switch(mode){
		case 0:
			buffer[1] =0x82;
			dsi_set_cmd_by_cmdq(handle,0x55,1,&buffer[1],1);
			break;
		case 1:
			buffer[1] = 0x81;
			dsi_set_cmd_by_cmdq(handle,0x55,1,&buffer[1],1);
			break;
		case 2:
			buffer[1] = 0x82;
			dsi_set_cmd_by_cmdq(handle,0x55,1,&buffer[1],1);
			break;
		case 3:
			buffer[1] = 0x83;
			dsi_set_cmd_by_cmdq(handle,0x55,1,&buffer[1],1);
			break;
		default:
			break;
	}
	printk("setcabcmode end = 0x%x\n",buffer[1]);
	MDELAY(10);
	//arry[0] =0x00043700;
	//dsi_set_cmdq(arry, 1, 4);
	//read_reg_v2(0x56, buffer,1);
	//printk("setcabcmode after = 0x%x\n",buffer[0]);
#endif
#endif
}

static void lcm_setiemode(void* handle,unsigned int mode)
{
#if 0
#ifndef BUILD_LK
	unsigned char buffer[8] = {0x0};
       unsigned int arry[4] ;
	arry[0] =0x00043700;
	dsi_set_cmdq(arry, 1, 4);
	read_reg_v2(0x56, buffer,1);
	printk("setiemode begin = 0x%x,mode = %d\n",buffer[0],mode);
	switch(mode){
		case 0:
			//buffer[1] = ((buffer[0]<<4)>>4)|0x80;
			buffer[1] = 0x83;
			dsi_set_cmdq_V2(0x55,1,&buffer[1],1);
			printk("setiemode 0mode = %d\n",mode);
			break;
		case 1:
			//buffer[1] = ((buffer[0]<<4)>>4)|0xB0;
			buffer[1] = 0x83;// 0xb3
			dsi_set_cmdq_V2(0x55,1,&buffer[1],1);
			printk("setiemode 1mode = %d\n",mode);
			break;
		case 2:
			buffer[1] = ((buffer[0]<<4)>>4)|0x90;
			dsi_set_cmdq_V2(0x55,1,&buffer[1],1);
			break;
		case 3:
			buffer[1] = ((buffer[0]<<4)>>4)|0xB0;
			dsi_set_cmdq_V2(0x55,1,&buffer[1],1);
			break;
		case 4:
			buffer[1] = ((buffer[0]<<4)>>4)|0x40;
			dsi_set_cmdq_V2(0x55,1,&buffer[1],1);
			break;
		case 5:
			buffer[1] = ((buffer[0]<<4)>>4)|0x50;
			dsi_set_cmdq_V2(0x55,1,&buffer[1],1);
			break;
		case 6:
			buffer[1] = ((buffer[0]<<4)>>4)|0x60;
			dsi_set_cmdq_V2(0x55,1,&buffer[1],1);
			break;
		default:
			break;
	}
	printk("setiemode end = 0x%x\n",buffer[1]);
	MDELAY(10);
	arry[0] =0x00043700;
	dsi_set_cmdq(arry, 1, 4);
	read_reg_v2(0x56, buffer,1);
	printk("setiemode after = 0x%x\n",buffer[0]);
#endif
#endif
}

#endif
#endif
/* lenovo-sw yexm1 add lcd effect function 20140314 end */
void* lcm_switch_mode(int mode)
{
#ifndef BUILD_LK
//customization: 1. V2C config 2 values, C2V config 1 value; 2. config mode control register
	if(mode == 0)
	{//V2C
		lcm_switch_mode_cmd.mode = CMD_MODE;
		lcm_switch_mode_cmd.addr = 0xBB;// mode control addr
		lcm_switch_mode_cmd.val[0]= 0x13;//enabel GRAM firstly, ensure writing one frame to GRAM
		lcm_switch_mode_cmd.val[1]= 0x10;//disable video mode secondly
	}
	else
	{//C2V
		lcm_switch_mode_cmd.mode = SYNC_PULSE_VDO_MODE;
		lcm_switch_mode_cmd.addr = 0xBB;
		lcm_switch_mode_cmd.val[0]= 0x03;//disable GRAM and enable video mode
	}
	return (void*)(&lcm_switch_mode_cmd);
#else
	return NULL;
#endif
}

void lcm_set_cmd(void* handle,unsigned int mode)
{
#ifdef BUILD_LK
	//dprintf(0,"%s,lk nt35595 set cmd: mode = %d\n", __func__, mode);
#else
	
//customize
	unsigned  cmd = 0x51;
	unsigned char  count =1;
    unsigned char level = mode;
	//printk("%s, kernel nt35595 set cmd: mode = %d\n", __func__, mode);
	//printk("[lcm_set_cmd mode = %d\n", mode);
	dsi_set_cmd_by_cmdq(handle, cmd, count, &level, 1);
	//lcm_util.dsi_set_cmdq_V22(handle, cmd, count, &level, 1);
#endif
}

LCM_DRIVER nt35595_fhd_dsi_vdo_lg_lcm_drv=
{
    .name           = "nt35595_fhd_dsi_vdo_lg",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,/*tianma init fun.*/
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
     .compare_id     = lcm_compare_id,
#if (LCM_DSI_CMD_MODE)
    .update         = lcm_update,
#endif
//#ifdef LENOVO_LCD_BACKLIGHT_CONTROL_BY_LCM
	.set_backlight		= lcm_setbacklight,	//Lenovo-sw yexm1 add 20140402
//#endif
/* lenovo-sw yexm1 add lcd effect function 20140314 start */
#ifndef BUILD_LK
#ifdef CONFIG_LENOVO_LCM_EFFECT
	.set_gammamode = lcm_setgammamode,
	.set_inversemode = lcm_setinversemode,
	//.set_inversemode = lcm_setcabcmode,
	.set_cabcmode = lcm_setcabcmode,
	.set_iemode = lcm_setiemode,
#endif
#endif
     .switch_mode		= lcm_switch_mode,
     .set_cmd = lcm_set_cmd,
/* lenovo-sw yexm1 add lcd effect function 20140314 end */
};
/* END PN:DTS2013053103858 , Added by d00238048, 2013.05.31*/
