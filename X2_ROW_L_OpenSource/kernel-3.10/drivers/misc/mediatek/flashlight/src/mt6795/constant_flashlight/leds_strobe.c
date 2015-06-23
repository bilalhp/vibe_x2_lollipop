#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/time.h>
#include "kd_flashlight.h"
#include <asm/io.h>
#include <asm/uaccess.h>
#include "kd_camera_hw.h"
#include <cust_gpio_usage.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/xlog.h>
#include <linux/version.h>
#include <linux/leds.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
#include <linux/mutex.h>
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif
#endif

//add i2c driver.
#include <linux/i2c.h>
#include <linux/platform_device.h>

#define LM3642_I2C_BUSNUM 2 
#define LM3642_DEVICE_ID 0xc6 //8 bit addr.
#define LM3642_DRVNAME "LM3642"
#define FLASH_GPIO_ENF GPIO_CAMERA_FLASH_EN_PIN
#define FLASH_GPIO_ENT GPIO_CAMERA_FLASH_MODE_PIN

static struct i2c_board_info __initdata kd_lm3642_dev={ I2C_BOARD_INFO("LM3642", LM3642_DEVICE_ID>>1)};
static const struct i2c_device_id LM3642_i2c_id[] = {{LM3642_DRVNAME,0},{}};
static struct i2c_client * g_pstI2Cclient = NULL;
static struct work_struct flash_work;
static void flash_work_func(struct work_struct *data);
static struct led_classdev flash_cdev;
static struct led_classdev torch_cdev;
static int fl_value=0;
static int flash_flag=false;

static int lm3642_write(u16 internal_addr,u8 data)
{
	int  i4RetValue = 0;
	char puSendCmd[2]={(char)(internal_addr&0xff),(char)data};
	printk("------[%s][%d]---------------\n",__FUNCTION__,__LINE__);
	if(g_pstI2Cclient==NULL)
	{
		printk("------[%s][%d]---------------\n",__FUNCTION__,__LINE__);
		return -1;
	}
	g_pstI2Cclient->addr = LM3642_DEVICE_ID>>1;
	g_pstI2Cclient->ext_flag = (g_pstI2Cclient->ext_flag)&(~I2C_DMA_FLAG);
	i4RetValue = i2c_master_send(g_pstI2Cclient, puSendCmd,2);
	if (i4RetValue != 2)
	{
		printk("[flash LM3642] I2C write  failed!! \n");
		return -1;
	}
	return 0;
}
static int lm3642_read(u16 internal_addr, u8 *data)
{
	int  i4RetValue = 0;
	char puReadCmd =(char)internal_addr;
	if(g_pstI2Cclient==NULL)
	{
		printk("------[%s][%d]---------------\n",__FUNCTION__,__LINE__);
		return -1;
	}
	g_pstI2Cclient->addr = LM3642_DEVICE_ID>>1;
	g_pstI2Cclient->ext_flag = (g_pstI2Cclient->ext_flag)&(~I2C_DMA_FLAG);
	i4RetValue = i2c_master_send(g_pstI2Cclient, puReadCmd, 1);
	if (i4RetValue != 1) {
		printk("[LM3642] I2C send failed, addr = 0x%x!! \n", internal_addr );
		return -1;
	}
	//
	i4RetValue = i2c_master_recv(g_pstI2Cclient, (char *)data, 1);
	if (i4RetValue != 1) {
		printk("[LM3642] I2C read failed!! \n");
		return -1;
	}

	return 0;
}
static int LM3642_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id) {             
	int i4RetValue = 0;
	g_pstI2Cclient = client;
	g_pstI2Cclient->addr = LM3642_DEVICE_ID>>1;
	
	printk("[flash LM3642] Attached!!-----------\n");
	return 0;                                                                                       
} 

static int LM3642_i2c_remove(struct i2c_client *client)
{
	return 0;
}

static struct i2c_driver LM3642_i2c_driver = {
	.probe = LM3642_i2c_probe,                                   
	.remove = LM3642_i2c_remove,                           
	.driver.name = LM3642_DRVNAME,
	.id_table = LM3642_i2c_id,                             
};

static void flash_work_func(struct work_struct *data)
{
	if(flash_flag){
		printk("--[%s][%d]--------\n",__FUNCTION__,__LINE__);
		if(fl_value!=0){
			lm3642_write(0x08,0x07);//timeout 600ms.
			lm3642_write(0x09,0x0d);//set flash level.
			lm3642_write(0x0a,0x03);//strobe pin disable ,flash mode.
		}else{
			lm3642_write(0x0a,0x00);//
		}
	}else{
		printk("--[%s][%d]--------\n",__FUNCTION__,__LINE__);
		if(fl_value!=0){
			lm3642_write(0x09,0x20);//set torch level.
			lm3642_write(0x0a,0x10);//torch pin enable,torch mode.
			mt_set_gpio_out(FLASH_GPIO_ENT,GPIO_OUT_ONE);
		}else{
			mt_set_gpio_out(FLASH_GPIO_ENT,GPIO_OUT_ZERO);
			lm3642_write(0x0a,0x00);//torch pin disable.
		}
	}
	return;
}

void lm3642flash_brightness_set(struct led_classdev *led_cdev,enum led_brightness brightness)
{
	//ii
	printk("--[%s][%d]------%d--\n",__FUNCTION__,__LINE__,brightness);
	fl_value=brightness;
	flash_flag=true;
	schedule_work(&flash_work);
		
	return;
}

void lm3642torch_brightness_set(struct led_classdev *led_cdev,enum led_brightness brightness)
{
	printk("--[%s][%d]------%d--\n",__FUNCTION__,__LINE__,brightness);
	fl_value=brightness;
	flash_flag=false;
	schedule_work(&flash_work);
	return;
}
static int LM3642_probe(struct platform_device *pdev)
{

	//create some sys node.
	torch_cdev.name="torch";
	torch_cdev.brightness_set=lm3642torch_brightness_set;
	led_classdev_register(&(pdev->dev),&torch_cdev);

	flash_cdev.name="flash";
	flash_cdev.brightness_set=lm3642flash_brightness_set;
	led_classdev_register(&(pdev->dev),&flash_cdev);

	INIT_WORK(&flash_work, flash_work_func);

	return i2c_add_driver(&LM3642_i2c_driver);

}

static int LM3642_remove(struct platform_device *pdev)
{
 	led_classdev_unregister(&torch_cdev);
 	led_classdev_unregister(&flash_cdev);
	i2c_del_driver(&LM3642_i2c_driver);
	return 0;
}

// platform structure
static struct platform_driver g_LM3642_Driver = {
	.probe		= LM3642_probe,
	.remove	= LM3642_remove,
	.driver		= {
		.name	= LM3642_DRVNAME,
		.owner	= THIS_MODULE,
	}
};


static struct platform_device g_LM3642_Device = {
	.name = LM3642_DRVNAME,
	.id = 0,
	.dev = {
	}
};

static int __init LM3642_i2C_init(void)
{
	i2c_register_board_info(LM3642_I2C_BUSNUM, &kd_lm3642_dev, 1);
	if(platform_driver_register(&g_LM3642_Driver)){
		printk("failed to register LM3642 driver\n");
		return -ENODEV;
	}

	if (platform_device_register(&g_LM3642_Device))
	{
		printk("failed to register LM3642 driver, 2nd time\n");
		return -ENODEV;
	}	

	return 0;
}

static void __exit LM3642_i2C_exit(void)
{
	platform_driver_unregister(&g_LM3642_Driver);
}

module_init(LM3642_i2C_init);
module_exit(LM3642_i2C_exit);

//endi i2c driver

/******************************************************************************
 * Debug configuration
 ******************************************************************************/
// availible parameter
// ANDROID_LOG_ASSERT
// ANDROID_LOG_ERROR
// ANDROID_LOG_WARNING
// ANDROID_LOG_INFO
// ANDROID_LOG_DEBUG
// ANDROID_LOG_VERBOSE
#define TAG_NAME "leds_strobe.c"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    xlog_printk(ANDROID_LOG_DEBUG  , TAG_NAME, KERN_INFO  "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_WARN(fmt, arg...)        xlog_printk(ANDROID_LOG_WARNING, TAG_NAME, KERN_WARNING  "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_NOTICE(fmt, arg...)      xlog_printk(ANDROID_LOG_DEBUG  , TAG_NAME, KERN_NOTICE  "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_INFO(fmt, arg...)        xlog_printk(ANDROID_LOG_INFO   , TAG_NAME, KERN_INFO  "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_TRC_FUNC(f)              xlog_printk(ANDROID_LOG_DEBUG  , TAG_NAME,  "<%s>\n", __FUNCTION__);
#define PK_TRC_VERBOSE(fmt, arg...) xlog_printk(ANDROID_LOG_VERBOSE, TAG_NAME,  fmt, ##arg)
#define PK_ERROR(fmt, arg...)       xlog_printk(ANDROID_LOG_ERROR  , TAG_NAME, KERN_ERR "%s: " fmt, __FUNCTION__ ,##arg)


#define DEBUG_LEDS_STROBE
#ifdef  DEBUG_LEDS_STROBE
#define PK_DBG PK_DBG_FUNC
#define PK_VER PK_TRC_VERBOSE
#define PK_ERR PK_ERROR
#else
#define PK_DBG(a,...)
#define PK_VER(a,...)
#define PK_ERR(a,...)
#endif

/******************************************************************************
 * local variables
 ******************************************************************************/

static DEFINE_SPINLOCK(g_strobeSMPLock); /* cotta-- SMP proection */


static u32 strobe_Res = 0;
static u32 strobe_Timeus = 0;
static BOOL g_strobe_On = 0;

static int g_duty=0;
static int g_timeOutTimeMs=0;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
static DEFINE_MUTEX(g_strobeSem);
#else
static DECLARE_MUTEX(g_strobeSem);
#endif

static struct work_struct workTimeOut;

#define FLASH_TORCH_SWITCH  7 
/*****************************************************************************
  Functions
 *****************************************************************************/
static void work_timeOutFunc(struct work_struct *data);

int FL_Enable(void)
{
	printk("------[%s][%d]--------[%d]-------\n",__FUNCTION__,__LINE__,g_duty);
	if(g_duty>0)
	{
		printk("------[%s][%d]------flash-------\n",__FUNCTION__,__LINE__);
		if(g_duty>16)
		{
			printk("flash mode g_duty error,---duty=[%d]-\n",g_duty);
			g_duty=16;
		}
		lm3642_write(0x08,0x07);//time out 800ms
		lm3642_write(0x09,g_duty-1);//set flash level.
		lm3642_write(0x0a,0x03);//strobe pin disable ,flash mode.
		//flash mode.
	}else{
		printk("------[%s][%d]-------------\n",__FUNCTION__,__LINE__);
		if(g_duty<0)
		{
			printk("torch mode g_duty error,---duty=[%d]-\n",g_duty);
			return -1;
		}
		lm3642_write(0x09,0x20);//set torch level.
		lm3642_write(0x0a,0x02);//torch pin disable,torch mode.
		//torch mode.
	}
	return 0;
}
int FL_Disable(void)
{
	printk("------[%s][%d]---------------\n",__FUNCTION__,__LINE__);
	lm3642_write(0x0a,0x00);//torch pin enable,torch mode.
	
	return 0;
}

int FL_dim_duty(kal_uint32 duty)
{
	PK_DBG("-------------FL_dim_duty line=%d\n",__LINE__);
	g_duty =  duty;
	return 0;
}

int FL_Init(void)
{
	if(mt_set_gpio_mode(FLASH_GPIO_ENT,GPIO_MODE_00)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
	if(mt_set_gpio_dir(FLASH_GPIO_ENT,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
	if(mt_set_gpio_out(FLASH_GPIO_ENT,GPIO_OUT_ZERO)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}
#if 0
	if(mt_set_gpio_mode(FLASH_GPIO_ENF,GPIO_MODE_00)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
	if(mt_set_gpio_dir(FLASH_GPIO_ENF,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
	if(mt_set_gpio_out(FLASH_GPIO_ENF,GPIO_OUT_ZERO)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}
#endif
	INIT_WORK(&workTimeOut, work_timeOutFunc);
	PK_DBG(" FL_Init line=%d\n",__LINE__);
	return 0;
}

int FL_Uninit(void)
{
	FL_Disable();
	return 0;
}

/*****************************************************************************
  User interface
 *****************************************************************************/

static void work_timeOutFunc(struct work_struct *data)
{
	FL_Disable();
	PK_DBG("ledTimeOut_callback\n");
}

enum hrtimer_restart ledTimeOutCallback(struct hrtimer *timer)
{
	schedule_work(&workTimeOut);
	return HRTIMER_NORESTART;
}
static struct hrtimer g_timeOutTimer;
void timerInit(void)
{
	g_timeOutTimeMs=1000; //1s
	hrtimer_init( &g_timeOutTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
	g_timeOutTimer.function=ledTimeOutCallback;

}
static int constant_flashlight_ioctl(MUINT32 cmd, MUINT32 arg)
{
	int i4RetValue = 0;
	int ior_shift;
	int iow_shift;
	int iowr_shift;
	ior_shift = cmd - (_IOR(FLASHLIGHT_MAGIC,0, int));
	iow_shift = cmd - (_IOW(FLASHLIGHT_MAGIC,0, int));
	iowr_shift = cmd - (_IOWR(FLASHLIGHT_MAGIC,0, int));
	PK_DBG("constant_flashlight_ioctl() line=%d ior_shift=%d, iow_shift=%d iowr_shift=%d arg=%d\n",__LINE__, ior_shift, iow_shift, iowr_shift, arg);
	switch(cmd)
	{

		case FLASH_IOC_SET_TIME_OUT_TIME_MS:
			PK_DBG("FLASH_IOC_SET_TIME_OUT_TIME_MS: %d\n",arg);
			g_timeOutTimeMs=arg;
			break;


		case FLASH_IOC_SET_DUTY :
			PK_DBG("----------FLASHLIGHT_DUTY: %d\n",arg);
			FL_dim_duty(arg);
			break;


		case FLASH_IOC_SET_STEP:
			PK_DBG("FLASH_IOC_SET_STEP: %d\n",arg);
			break;

		case FLASH_IOC_SET_ONOFF :
			PK_DBG("FLASHLIGHT_ONOFF: %d\n",arg);
			if(arg==1)
			{
				if(g_timeOutTimeMs!=0)
				{
					ktime_t ktime;
					ktime = ktime_set( 0, g_timeOutTimeMs*1000000 );
					hrtimer_start( &g_timeOutTimer, ktime, HRTIMER_MODE_REL );
				}
				FL_Enable();
			}
			else
			{
				//FL_Enable();
				FL_Disable();
				hrtimer_cancel( &g_timeOutTimer );
			}
			break;
		default :
			PK_DBG(" No such command \n");
			i4RetValue = -EPERM;
			break;
	}
	return i4RetValue;
}

static int constant_flashlight_open(void *pArg)
{
	int i4RetValue = 0;
	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);

	if (0 == strobe_Res)
	{
		FL_Init();
		timerInit();
	}
	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);
	spin_lock_irq(&g_strobeSMPLock);


	if(strobe_Res)
	{
		PK_ERR(" busy!\n");
		i4RetValue = -EBUSY;
	}
	else
	{
		strobe_Res += 1;
	}


	spin_unlock_irq(&g_strobeSMPLock);
	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);

	return i4RetValue;

}


static int constant_flashlight_release(void *pArg)
{
	PK_DBG(" constant_flashlight_release\n");

	if (strobe_Res)
	{
		spin_lock_irq(&g_strobeSMPLock);

		strobe_Res = 0;
		strobe_Timeus = 0;

		/* LED On Status */
		g_strobe_On = FALSE;

		spin_unlock_irq(&g_strobeSMPLock);

		FL_Uninit();
	}

	PK_DBG(" Done\n");

	return 0;

}


FLASHLIGHT_FUNCTION_STRUCT	constantFlashlightFunc=
{
	constant_flashlight_open,
	constant_flashlight_release,
	constant_flashlight_ioctl
};


MUINT32 constantFlashlightInit(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc)
{
	if (pfFunc != NULL)
	{
		*pfFunc = &constantFlashlightFunc;
	}
	return 0;
}


/* LED flash control for high current capture mode*/
ssize_t strobe_VDIrq(void)
{

	return 0;
}

EXPORT_SYMBOL(strobe_VDIrq);
