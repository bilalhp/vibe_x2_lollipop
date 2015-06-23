#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/xlog.h>
#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_camera_feature.h"

#include <asm/io.h>
#include <linux/spinlock.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_pm_ldo.h>

static int hwCameraPowerOn(MT65XX_POWER powerId, MT65XX_POWER_VOLTAGE powerVolt)
{
	if (  (powerId == MT6331_POWER_LDO_VAUD32) 
			||(powerId == MT6331_POWER_LDO_VAUXA32) 
			||(powerId == MT6331_POWER_LDO_VCAMA)
			||(powerId == MT6331_POWER_LDO_VMCH)
			||(powerId == MT6331_POWER_LDO_VEMC33)
			||(powerId == MT6331_POWER_LDO_VMC)
			||(powerId == MT6331_POWER_LDO_VCAM_AF)
			||(powerId == MT6331_POWER_LDO_VGP1)
			||(powerId == MT6331_POWER_LDO_VGP4)
			||(powerId == MT6331_POWER_LDO_VSIM1)
			||(powerId == MT6331_POWER_LDO_VSIM2)    	
			||(powerId == MT6331_POWER_LDO_VMIPI)    	
			||(powerId == MT6331_POWER_LDO_VIBR)
			||(powerId == MT6331_POWER_LDO_VDIG18)    	
			||(powerId == MT6331_POWER_LDO_VCAMD)
			||(powerId == MT6331_POWER_LDO_VUSB10)
			||(powerId == MT6331_POWER_LDO_VCAM_IO)
			||(powerId == MT6331_POWER_LDO_VSRAM_DVFS1)
			||(powerId == MT6331_POWER_LDO_VGP2)
			||(powerId == MT6331_POWER_LDO_VGP3)
			||(powerId == MT6331_POWER_LDO_VBIASN)
			||(powerId == MT6332_POWER_LDO_VAUXB32)
			||(powerId == MT6332_POWER_LDO_VDIG18)
			||(powerId == MT6332_POWER_LDO_VSRAM_DVFS2)
			)
			{
				pmic_ldo_vol_sel(powerId, powerVolt);
			}
	pmic_ldo_enable(powerId, KAL_TRUE);
	return 0;    
}
static int hwCameraPowerDown(MT65XX_POWER powerId)
{
	pmic_ldo_enable(powerId, KAL_FALSE);
	return 0;    
}
/******************************************************************************
 * Debug configuration
 ******************************************************************************/
#define PFX "[kd_camera_hw]"
//#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    xlog_printk(ANDROID_LOG_INFO, PFX , fmt, ##arg)

#define DEBUG_CAMERA_HW_K
#ifdef DEBUG_CAMERA_HW_K
#define PK_DBG PK_DBG_FUNC
#define PK_ERR(fmt, arg...)         xlog_printk(ANDROID_LOG_ERR, PFX , fmt, ##arg)
#define PK_XLOG_INFO(fmt, args...) \
	do {    \
		xlog_printk(ANDROID_LOG_INFO, PFX , fmt, ##arg); \
	} while(0)
#else
#define PK_DBG(a,...)
#define PK_ERR(a,...)
#define PK_XLOG_INFO(fmt, args...)
#endif

#ifndef BOOL
typedef unsigned char BOOL;
#endif
#define IMX214_CORE_SUPPLY_GPIO (GPIO104|0x80000000)
#define IMX214_DOVDD_SUPPLY_GPIO (GPIO105|0x80000000)
#define CAMERA_MCLK_GPIO (GPIO42|0x80000000)
#define CAMERA_MCLK3_GPIO (GPIO40|0x80000000)
extern void ISP_MCLK1_EN(BOOL En);
extern void ISP_MCLK2_EN(BOOL En);
extern void ISP_MCLK3_EN(BOOL En);

static int CameraPowerOn(void)
{
	//set back camera pwdn off  mode.
	if(mt_set_gpio_mode(CAMERA_CMPDN_PIN,CAMERA_CMRST_PIN_M_GPIO))
		PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");
	if(mt_set_gpio_dir(CAMERA_CMPDN_PIN,GPIO_DIR_OUT))
		PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");
	if(mt_set_gpio_out(CAMERA_CMPDN_PIN,CAMERA_CMPDN_PIN_OFF))
		PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");

	//set front camera pwdn off  mode.
	if(mt_set_gpio_mode(CAMERA_CMPDN1_PIN,CAMERA_CMRST_PIN_M_GPIO))
		PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");
	if(mt_set_gpio_dir(CAMERA_CMPDN1_PIN,GPIO_DIR_OUT))
		PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");
	if(mt_set_gpio_out(CAMERA_CMPDN1_PIN,CAMERA_CMPDN_PIN_OFF))
		PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");

	//power on back camera then front.
	//power on avdd iovdd dvdd
	hwCameraPowerOn(CAMERA_POWER_VCAM_A, VOL_2800);
	hwCameraPowerOn(CAMERA_POWER_VCAM_D, VOL_1800);

	//dovdd control by gpio dc-dc.
	if(mt_set_gpio_mode(IMX214_DOVDD_SUPPLY_GPIO,GPIO_MODE_00))
		PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");
	if(mt_set_gpio_dir(IMX214_DOVDD_SUPPLY_GPIO,GPIO_DIR_OUT))
		PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");
	if(mt_set_gpio_out(IMX214_DOVDD_SUPPLY_GPIO,GPIO_OUT_ONE))
		PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");

	//digtal core 1.0v gpio control dc-dc supply.
	if(mt_set_gpio_mode(IMX214_CORE_SUPPLY_GPIO,GPIO_MODE_00))
		PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");
	if(mt_set_gpio_dir(IMX214_CORE_SUPPLY_GPIO,GPIO_DIR_OUT))
		PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");
	if(mt_set_gpio_out(IMX214_CORE_SUPPLY_GPIO,GPIO_OUT_ONE))
		PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");

	//set front camera power on
	hwCameraPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800);
	hwCameraPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800);
	hwCameraPowerOn(CAMERA_POWER_VCAM_DC2,VOL_1200);
	
	return  0;
}

static int CameraPowerOff(void)
{
	//power off, back camera then front.
	//dvdd.
	if(mt_set_gpio_out(IMX214_CORE_SUPPLY_GPIO,GPIO_OUT_ZERO))
		PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");
	//dovdd and avdd
	hwCameraPowerDown(CAMERA_POWER_VCAM_A);
	hwCameraPowerDown(CAMERA_POWER_VCAM_D);

	//dovdd control by gpio dc-dc.
	if(mt_set_gpio_out(IMX214_DOVDD_SUPPLY_GPIO,GPIO_OUT_ZERO))
		PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");

	//front camera.
	hwCameraPowerDown(CAMERA_POWER_VCAM_DC2);
	hwCameraPowerDown(CAMERA_POWER_VCAM_A2);
	hwCameraPowerDown(CAMERA_POWER_VCAM_D2);

	return 0;
}

static void camera_mmclk_enable(void)
{
	mt_set_gpio_mode(CAMERA_MCLK_GPIO,GPIO_MODE_01);
	ISP_MCLK1_EN(1);
	return;
}

static void camera_mmclk_disable(void)
{
	ISP_MCLK1_EN(0);

	mt_set_gpio_mode(CAMERA_MCLK_GPIO,GPIO_MODE_00);
	mt_set_gpio_dir(CAMERA_MCLK_GPIO,GPIO_DIR_OUT);
	mt_set_gpio_out(CAMERA_MCLK_GPIO,GPIO_OUT_ZERO);
	return;
}
static void camera_mmclk3_enable(void)
{
	mt_set_gpio_mode(CAMERA_MCLK3_GPIO,GPIO_MODE_01);
	ISP_MCLK3_EN(1);
	return;
}
static void camera_mmclk3_disable(void)
{
	ISP_MCLK3_EN(0);

	mt_set_gpio_mode(CAMERA_MCLK3_GPIO,GPIO_MODE_00);
	mt_set_gpio_dir (CAMERA_MCLK3_GPIO,GPIO_DIR_OUT);
	mt_set_gpio_out (CAMERA_MCLK3_GPIO,GPIO_OUT_ZERO);
	return;
}
static BOOL main_sensor=false;
static BOOL sub_sensor=false;
int kdCISModulePowerOn(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On, char* mode_name)
{
	u32 pinSetIdx = 0;//default main sensor

	//return 0; //just for test ldo_cam_io status abnormal.

	if (DUAL_CAMERA_MAIN_SENSOR == SensorIdx)
		pinSetIdx = 0;
	else if (DUAL_CAMERA_SUB_SENSOR == SensorIdx) 
		pinSetIdx = 1;
	else if (DUAL_CAMERA_MAIN_2_SENSOR == SensorIdx) 
		pinSetIdx = 2;

	PK_DBG("--mclk new----sensor index=[%d]--sensor=[%s]----mode=[%s]--on=[%d]\n",pinSetIdx,currSensorName,mode_name,On);
	//power ON
	if (On) {
		//main camera power on
		if(DUAL_CAMERA_MAIN_SENSOR == SensorIdx)
		{
			if(sub_sensor)
			{
				PK_DBG("---[%s][%d]-------power on----\n",__FUNCTION__,__LINE__);
				//keep sub sensor and power on main sensor.
				mdelay(2);
				camera_mmclk_enable();
				mdelay(2);
				if(mt_set_gpio_out(CAMERA_CMPDN_PIN,CAMERA_CMPDN_PIN_ON))
					PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");
			}else{
				PK_DBG("---[%s][%d]-------power on----\n",__FUNCTION__,__LINE__);

				CameraPowerOn();
				
				//set main camera power on
				//ISP_MCLK1_EN(1);
				mdelay(2);
				camera_mmclk_enable();
				mdelay(2);
				if(mt_set_gpio_out(CAMERA_CMPDN_PIN,CAMERA_CMPDN_PIN_ON))
					PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");

				//set sub camera power down.
				if(mt_set_gpio_out(CAMERA_CMPDN1_PIN,CAMERA_CMPDN1_PIN_OFF))
					PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");
			}
			//set vcm on
			hwCameraPowerOn(CAMERA_POWER_AF,VOL_2800);
			main_sensor=true;
		}
		//sub camera power on
		if (DUAL_CAMERA_SUB_SENSOR == SensorIdx)
		{
			if(main_sensor){
				PK_DBG("---[%s][%d]-------power on----\n",__FUNCTION__,__LINE__);
				//keep main sensor power not chenage and power on sub sensor. 
				camera_mmclk3_enable();
				if(mt_set_gpio_out(CAMERA_CMPDN1_PIN,CAMERA_CMPDN1_PIN_ON))
					PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");
			}else{
				PK_DBG("---[%s][%d]-------power on----\n",__FUNCTION__,__LINE__);

				CameraPowerOn();
				
				//set sub camera on
				//ISP_MCLK3_EN(1);
				camera_mmclk3_enable();
				if(mt_set_gpio_out(CAMERA_CMPDN1_PIN,CAMERA_CMPDN1_PIN_ON))
					PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");

				//set main camera power down.	
				if(mt_set_gpio_out(CAMERA_CMPDN_PIN,CAMERA_CMPDN_PIN_OFF))
					PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");
			}
			sub_sensor=true;
		}
	}else {//power OFF
		if(pinSetIdx == 0 ) 
			//ISP_MCLK1_EN(0);
			camera_mmclk_disable();
		else if (pinSetIdx == 1)	
			//ISP_MCLK3_EN(0);
			camera_mmclk3_disable();
		else if (pinSetIdx == 2)
			ISP_MCLK2_EN(0);

		//main sensor power off
		if(DUAL_CAMERA_MAIN_SENSOR == SensorIdx)
		{
			if(sub_sensor){
				PK_DBG("---[%s][%d]-------power off----\n",__FUNCTION__,__LINE__);
				//set main sensor power down and keep avdd and dovdd.
				if(mt_set_gpio_out(CAMERA_CMPDN_PIN,CAMERA_CMPDN_PIN_OFF))
					PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");
			}else{		
				PK_DBG("---[%s][%d]-------power off----\n",__FUNCTION__,__LINE__);
				
				//set main camera off.
				if(mt_set_gpio_out(CAMERA_CMPDN_PIN,CAMERA_CMPDN_PIN_OFF))
					PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");

				CameraPowerOff();
			}
			//power off vcm.
			hwCameraPowerDown(CAMERA_POWER_AF);

			main_sensor=false;
		}

		//sub sensor power off
		if (DUAL_CAMERA_SUB_SENSOR == SensorIdx)
		{
			if(main_sensor){
				PK_DBG("---[%s][%d]-------power off----\n",__FUNCTION__,__LINE__);
				//set sub sensor power down and keep dovdd and avdd. 
				mt_set_gpio_out(CAMERA_CMPDN1_PIN,CAMERA_CMPDN1_PIN_OFF);
			}else{
				PK_DBG("---[%s][%d]-------power off----\n",__FUNCTION__,__LINE__);

				//set sub camera off.
				mt_set_gpio_out(CAMERA_CMPDN1_PIN,CAMERA_CMPDN1_PIN_OFF);

				CameraPowerOff();
			}
			sub_sensor=false;
		}
	}//

	return 0;
}

EXPORT_SYMBOL(kdCISModulePowerOn);

