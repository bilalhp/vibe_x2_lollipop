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

static struct LCM_setting_table lcm_initialization_setting[] = {
//enable orise command
         {0x00,1,{0x00}},
         {0xFF,3,{0x19,0x02,0x01}},
         {0x00,1,{0x80}},
         {0xFF,2,{0x19,0x02}},
         //select LCD work mode. 00(Cmd mode)/13(Video mode)
         {0x00,1,{0x00}},
         {0x1C,1,{0x00}},
         //gamma load in sleep out
         {0x00,1,{0x83}},
         {0xF3,1,{0xC8}},
         //OSC & Pclk freq
         {0x00,1,{0x80}},
         {0xC1,2,{0x11,0x11}},
         {0x00,1,{0x90}},
         {0xC1,2,{0x44,0x00}},
         //add by tim
         {0x00,1,{0x80}},
         {0xC0,14,{0x00,0x77,0x00,0x42,0x43,0x00,0x77,0x42,0x43,0x00,0x77,0x00,0x42,0x43}},
         {0x44,2,{0x01,0x00}},

         {0x00,1,{0x8b}},
         {0xb0,1,{0x00}},

////////////////////////////// 
//add to reduce noise 20141030_2005


         //source 在 V-blanking推最小压差
         {0x00,1,{0x80}},
         {0xC4,1,{0x18}},
         
         //source Pre-charge enable
         {0x00,1,{0xF8}},
         {0xC2,1,{0x82}},


         //每条线时CKHR/G/B才开始动作
         {0x00,1,{0xFB}},
         {0xC2,1,{0x00}},



         //source pre-charge 时间
         {0x00,1,{0x82}},
         {0xC4,1,{0x00}},


         //VGLO :-10v -->-9v
         {0x00,1,{0x99}},
         {0xC5,1,{0x32}},//{0xC5,1,{0x28}},VGLO :-10v -->-8v


//add to reduce noise 20141030_2005 END
//////////////////////////////



         //end by tim
         //enable TE, just for cmd mode of mipi
         {0x35,1,{0x00}},
         //enable CABC function.
         //{0x53,1,{0x2C}},
		 
		 /////////***********Gamma(0.30,0.32)*******//////////
		{0x00,1,{0x00}},
 {0xE1,24,{0x00,  0x08,	0x0D,	0x19,	0x21,	0x26,	0x34,	0x45,	0x50,	0x62,	0x70,	0x7A,	0x7B,	0x71,	0x67,	0x55,	0x43,	0x31,	0x26,	0x1D,	0x14,	0x09,	0x06,	0x03}},
 {0x00,1,{0x00}},
 {0xE2,24,{0x00,  0x08,	0x0D,	0x19,	0x21,	0x26,	0x34,	0x45,	0x50,	0x62,	0x70,	0x7A,	0x7B,	0x71,	0x67,	0x55,	0x43,	0x31,	0x26,	0x1D,	0x14,	0x09,	0x06,	0x03}},
 {0x00,1,{0x00}},
 {0xE3,24,{0x28,	0x2A,	0x2D,	0x33,	0x37,	0x3A,	0x43,	0x51,	0x5A,	0x69,	0x73,	0x7D,	0x79,	0x70,	0x66,	0x54,	0x42,	0x31,	0x26,	0x1D,	0x14,	0x09,	0x06,	0x03}},
 {0x00,1,{0x00}},
 {0xE4,24,{0x28,	0x2A,	0x2D,	0x33,	0x37,	0x3A,	0x43,	0x51,	0x5A,	0x69,	0x73,	0x7D,	0x79,	0x6F,	0x66,	0x54,	0x42,	0x31,	0x26,	0x1D,	0x14,	0x09,	0x06,	0x03}},
 {0x00,1,{0x00}},
 {0xE5,24,{0x48,	0x49,	0x4A,	0x4C,	0x4F,	0x51,	0x57,	0x61,	0x67,	0x71,	0x79,	0x81,	0x76,	0x6D,	0x63,	0x4F,	0x37,	0x20,	0x1A,	0x17,	0x12,	0x09,	0x06,	0x03}},
 {0x00,1,{0x00}},
 {0xE6,24,{0x48,	0x49,	0x4A,	0x4C,	0x4F,	0x51,	0x57,	0x61,	0x67,	0x71,	0x79,	0x81,	0x76,	0x6D,	0x63,	0x4F,	0x37,	0x20,	0x1A,	0x17,	0x12,	0x09,	0x06,	0x03}},

{0x00,1,{0x00}},
{0xEC,33,{0x40,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x04}},

{0x00,1,{0x00}},
{0xED,33,{0x40,0x54,0x43,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x04}},

{0x00,1,{0x00}},
{0xEE,33,{0x40,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x04}},
		 
         //sleep out
         {0x11,0,{}},
         {REGFLAG_DELAY, 120, {}},
         //display on
         {0x29,0,{}},
         //write     display      brightness level to max.
         {0x51,1,{0xFF}},
         {REGFLAG_DELAY, 10, {}},
         {REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
    // Display off sequence
    {0x28, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    // Sleep Mode On
    {0x10, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_sleep_out_setting[] = {
    //Sleep Out
    {0x11, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
    {0x29, 1, {0x00}},
    {REGFLAG_DELAY, 20, {}},
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
#else
	params->dsi.mode   = SYNC_PULSE_VDO_MODE;
#endif

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
	params->dsi.PLL_CLOCK = 440; //this value must be in MTK suggested table
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

static void lcm_init(void)
{
	/*unsigned char cmd = 0x0;
	unsigned char data = 0xFF;
	int ret=0;
	cmd=0x00;
	data=0x0a;*/
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
	MDELAY(6);
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_MODE_00);
    	mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);
    	mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ONE);

	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(10);

	// when phone initial , config output high, enable backlight drv chip
	push_table2(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
	mt_set_gpio_out(GPIO_LCM_BL_EN, GPIO_OUT_ZERO);
	push_table2(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_MODE_00);
    	mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);
    	mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ZERO);
	MDELAY(6);
 	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
 	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
 	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ZERO);
}

static void lcm_resume(void)
{
	//lcm_init();
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
 	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
 	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);
	MDELAY(6);
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_MODE_00);
    	mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);
    	mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ONE);
	MDELAY(15);

	push_table2(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1); 
	mt_set_gpio_out(GPIO_LCM_BL_EN, GPIO_OUT_ONE);
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

/* Lenovo-sw yexm1 add 20140402 begin */
//#ifdef LENOVO_LCD_BACKLIGHT_CONTROL_BY_LCM
static void lcm_setbacklight(unsigned int level)
{
	unsigned char data_array[16];
	//if(isAAL == 1) return;
	if(level > 255)
	level = 255;

#ifndef BUILD_LK
	printk("[yexm1] %s level is %d \n",__func__,level);
#endif
	data_array[0] = level;
	dsi_set_cmdq_V2(0x51, 1, data_array, 1);
}
//#endif
/* Lenovo-sw yexm1 add 20140402 end */

/*lenovo-sw zhouyj5 add LCD automatic recognition mechanism. 20140521 start */
#define LENOVO_LCD_ID_PIN (GPIO171 | 0x80000000)
static unsigned int lcm_compare_id(void)
{
	unsigned  int ret = 1;
	ret = mt_get_gpio_in(LENOVO_LCD_ID_PIN);
#ifdef BUILD_LK
	if(0 == ret)
		dprintf(0, "[LK]tianma_otm1902a found\n");
#else
	if(0 == ret)
		printk("[KERNEL]tianma_otm1902a found\n");
#endif
	return (0 == ret) ? 1: 0;
}
/*lenovo-sw zhouyj5 add LCD automatic recognition mechanism. 20140521 end */	

/* lenovo-sw yexm1 add lcd effect function 20140314 start */
#ifndef BUILD_LK
#ifdef CONFIG_LENOVO_LCM_EFFECT
static unsigned int lcm_gammamode_index = 0;
#include "otm1902a_tianma_CT.h"

static void lcm_setgamma_default(handle)
{
push_table(handle,lcm_gamma_default_setting, sizeof(lcm_gamma_default_setting) / sizeof(struct LCM_setting_table), 1); 
}

static void lcm_setgamma_cosy(handle)
{
push_table(handle,lcm_gamma_cosy_setting, sizeof(lcm_gamma_cosy_setting) / sizeof(struct LCM_setting_table), 1); 
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

static void lcm_setgammamode(void* handle,unsigned int mode)
{

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
			printk("%s start=%d\n",__func__,mode);
			lcm_set_CT_0297_0312(handle);
			printk("%s end=%d\n",__func__,mode);
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
}

static void lcm_setiemode(void* handle,unsigned int mode)
{
}
#endif
#endif
/* lenovo-sw yexm1 add lcd effect function 20140314 end */

LCM_DRIVER otm1902a_fhd_dsi_cmd_tianma_lcm_drv=
{
    .name           = "otm1902a_fhd_dsi_cmd_tianma",
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
#ifndef BUILD_LK
#ifdef CONFIG_LENOVO_LCM_EFFECT
	.set_gammamode = lcm_setgammamode,
	.set_inversemode = lcm_setinversemode,
	//.set_inversemode = lcm_setcabcmode,
	.set_cabcmode = lcm_setcabcmode,
	.set_iemode = lcm_setiemode,
#endif
#endif

};
/* END PN:DTS2013053103858 , Added by d00238048, 2013.05.31*/
