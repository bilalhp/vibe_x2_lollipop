#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/input.h> /* BUS_I2C */
#include <linux/input-polldev.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/string.h>
#include <linux/earlysuspend.h>
//lenovo-sw add for i2c dma start
#include <mach/eint.h>
#include <linux/dma-mapping.h>
//lenovo-sw add for i2c dma end
#include "CwMcuSensor.h"
#include <linux/gpio.h>
#include <mach/mt_gpio.h>
#include <cust_gpio_usage.h>
#include <cust_eint.h>

/* GPIO for MCU control */
#define GPIO_CW_MCU_WAKE_UP		 (GPIO133 | 0x80000000)
#define GPIO_CW_MCU_INTERRUPT	39

#define ACK		0x79
#define NACK		0x1F

#define DPS_MAX			(1 << (16 - 1))

/* Input poll interval in milliseconds */
#define CWMCU_POLL_INTERVAL	10
#define CWMCU_POLL_MAX		200
#define CWMCU_POLL_MIN		10

#define CWMCU_MAX_OUTPUT_ID		(CW_SNAP+1)
#define CWMCU_MAX_OUTPUT_BYTE		(CWMCU_MAX_OUTPUT_ID * 7)
#define CWMCU_MAX_DRIVER_OUTPUT_BYTE		256

//uint32_t m_Rawdata[3]={1000,2000,3000};
//m_Rawdata[0] = 1000;
//m_Rawdata[1] = 2000;
//m_Rawdata[2] = 3000;


static u8 *CWI2CDMABuf_va = NULL;
static u64 CWI2CDMABuf_pa = NULL;

static DEFINE_MUTEX(cwmcu_lock);

/* turn on gpio interrupt if defined */
#define CWMCU_INTERRUPT

//lenovo sw wangyq13 add start

static struct CWMCU_platform_data cwmcu_plat_data = {
	.Acceleration_hwid = 8,
	.Acceleration_deviceaddr = 0x30,
	.Acceleration_axes = 2,
	.Magnetic_hwid = 7,
	.Magnetic_deviceaddr = 0x5C,
	.Magnetic_axes = 6,
	.Gyro_hwid = 8,
	.Gyro_deviceaddr = 0xD0,
	.Gyro_axes = 2,
};

//CWSTM32
static struct i2c_board_info __initdata CwMcuSensor_i2c3_boardinfo[] = {
        {
						.type       = "CwMcuSensor",
						.addr       = 0x3a,
						.platform_data  = &cwmcu_plat_data,
        },
};

//lenovo sw wangyq13 add end



struct CWMCU_data {
	struct i2c_client *client;
	struct input_polled_dev *input_polled;
	struct input_dev *input;
	struct timeval now;
	int mcu_mode;
	unsigned char Acceleration_hwid;
	unsigned char Acceleration_deviceaddr;
	unsigned char Acceleration_axes;
	unsigned char Magnetic_hwid;
	unsigned char Magnetic_deviceaddr;
	unsigned char Magnetic_axes;
	unsigned char Gyro_hwid;
	unsigned char Gyro_deviceaddr;
	unsigned char Gyro_axes;

	struct hrtimer timer;

	struct workqueue_struct *driver_wq;
	struct work_struct work;

	/* enable & batch list */
	uint32_t enabled_list;
	uint32_t batched_list;
	bool use_interrupt;
	int batch_enabled;
	int64_t sensor_timeout[CW_SENSORS_ID_END];
	int64_t current_timeout;
	int timeout_count;
	uint32_t interrupt_status;

	/* report time */
	int64_t	sensors_time[CW_SENSORS_ID_END];/* pre_timestamp(us) */
	int64_t time_diff[CW_SENSORS_ID_END];
	int64_t	report_period[CW_SENSORS_ID_END];/* ms */
	uint32_t update_list;

	/* power status */
	int power_on_list;

	/* debug */
	int debug_count;

	/* calibrator */
	uint8_t cal_cmd;
	uint8_t cal_type;
	uint8_t cal_id;

	int cmd;
	uint32_t addr;
	int len;
	int mcu_slave_addr;
	int firmwave_update_status;
	int cw_i2c_rw;	/* r = 0 , w = 1 */
	int cw_i2c_len;
	uint8_t cw_i2c_data[300];

	uint8_t pwrmod; /* 0 : clear power mode, 1 : normal suspend mode, 2 : gesture wake up mode */
        #if defined(CONFIG_HAS_EARLYSUSPEND)    
        struct early_suspend    early_drv;
        #endif 
};

struct CWMCU_data *sensor;

static int CWMCU_i2c_write(struct CWMCU_data *sensor, u8 reg_addr, u8 *data, u8 len)
{
	int dummy;
	int i;
	//printk(KERN_ERR "LENOVO1 i2c write reg_addr=%02x,data=%02x\n",reg_addr,data[0]);
	mutex_lock(&cwmcu_lock);
	sensor->client->addr = sensor->client->addr & I2C_MASK_FLAG;
	for (i = 0; i < len; i++) {
		dummy = i2c_smbus_write_byte_data(sensor->client, reg_addr++, data[i]);
		//printk(KERN_ERR "LENOVO2 i2c write data=%02x  ",data[i]);
		if (dummy < 0) {
			printk(KERN_ERR "LENOVO++++i2c write error =%d\n", dummy);
			mutex_unlock(&cwmcu_lock);
			return dummy;
		}
	}
	//printk(KERN_ERR "\nLENOVO------------i2c write OK =%d\n", dummy);
	mutex_unlock(&cwmcu_lock);
	return 0;
}

/* Returns the number of read bytes on success */
static int CWMCU_i2c_read(struct CWMCU_data *sensor, u8 reg_addr, u8 *data, u8 len)
{
#if 1
       int dummy,i;
	mutex_lock(&cwmcu_lock);
       CWI2CDMABuf_va[0] = reg_addr;
       sensor->client->addr = sensor->client->addr & I2C_MASK_FLAG | I2C_DMA_FLAG;
	   sensor->client->timing = 400;	
        //add
        //printk(KERN_ERR "CWMCU #1 sensor->client->addr=0x%x,CWI2CDMABuf_pa=0x%x\n",sensor->client->addr,CWI2CDMABuf_pa);
	dummy = i2c_master_send(sensor->client, CWI2CDMABuf_pa, 1);
	if(dummy < 0)
		{
			printk(KERN_ERR "-CWMCU-- LENOVO++++CWMCU_i2c_read write reg_addr error =%d\n", dummy);
			mutex_unlock(&cwmcu_lock);
			return dummy;
		}
//add
        //printk(KERN_ERR "CWMCU #1 sensor->client->addr=0x%x,CWI2CDMABuf_pa=0x%x,len=%d\n",sensor->client->addr,CWI2CDMABuf_pa,len);
        sensor->client->addr = sensor->client->addr & I2C_MASK_FLAG | I2C_DMA_FLAG;
sensor->client->timing = 400;	
	dummy = i2c_master_recv(sensor->client, CWI2CDMABuf_pa,len);
	//if(dummy < 0)
		       //printk(KERN_ERR "LENOVO++++CWMCU_i2c_read read  error =%d\n", dummy);
       //else
	   	//printk(KERN_ERR "\nLENOVO----CWMCU_i2c_read read OK =%d\n", dummy);

	 if(dummy < 0)
	    {
			printk(KERN_ERR "-CWMCU-- LENOVO++++CWMCU_i2c_read read  error =%d\n", dummy);
			mutex_unlock(&cwmcu_lock);
			return dummy;
		}

		for(i = 0; i < len; i++)
		{
			data[i] = CWI2CDMABuf_va[i];
		}
	mutex_unlock(&cwmcu_lock);
	 return dummy;
#endif
#if 0
       printk(KERN_ERR "CWMCU_i2c_read i2c read reg_addr=%02x,len=%02x\n", reg_addr,len);
	return i2c_smbus_read_i2c_block_data(sensor->client, reg_addr, len, data);
#endif
}
/*
//write format    1.slave address  2.len of data  3.data[0] 4.data[1]
static int CWMCU_write_block(struct CWMCU_data *sensor, u8 reg_addr, u8 *data, u8 len){
	int dummy;
	dummy = i2c_smbus_write_block_data(sensor->client, reg_addr, len, data);
	if (dummy<0) {
		pr_err("i2c write error =%d\n",dummy);
		return dummy;
	}
	return 0;
}
*/
/* write format    1.slave address  2.data[0]  3.data[1] 4.data[2] */
static int CWMCU_write_i2c_block(struct CWMCU_data *sensor, u8 reg_addr, u8 *data, u8 len)
{
#if 1
	int dummy;
	int i = 0;
	mutex_lock(&cwmcu_lock);
	CWI2CDMABuf_va[0] = reg_addr;
	for(i = 1 ; i <= len; i++)
	{
		CWI2CDMABuf_va[i] = data[i-1];
	}
	sensor->client->addr = sensor->client->addr & I2C_MASK_FLAG | I2C_DMA_FLAG;
	sensor->client->timing = 400;
	dummy = i2c_master_send(sensor->client, CWI2CDMABuf_pa, len);
	if (dummy < 0) {
		printk(KERN_ERR "CWMCU_write_i2c_block i2c write error =%d\n", dummy);
		mutex_unlock(&cwmcu_lock);
		return dummy;
	}
	//else 
		//printk(KERN_ERR "\nLENOVO----CWMCU_write_i2c_block write OK =%d\n", dummy);
	mutex_unlock(&cwmcu_lock);
	return 0;
#endif
#if 0
	int dummy;
	dummy = i2c_smbus_write_i2c_block_data(sensor->client, reg_addr, len, data);
	printk(KERN_ERR "CWMCU_write_i2c_block i2c write reg_addr=%02x,len=%02x\n", reg_addr,len);
	if (dummy < 0) {
		printk(KERN_ERR"CWMCU_write_i2c_block i2c write error =%d\n", dummy);
		return dummy;
	}
	return 0;
#endif
}

static int CWMCU_write_serial(u8 *data, int len)
{
	int dummy;
	int i = 0;
	mutex_lock(&cwmcu_lock);
	for(i = 0 ; i < len; i++)
	{
		CWI2CDMABuf_va[i] = data[i];
	}
	if(len <= 8)
	{
	sensor->client->timing = 400;	
	dummy = i2c_master_send(sensor->client, data, len);
	if (dummy < 0) {
		printk(KERN_ERR "-CWMCU-- LENOVO888888888 i2c write error =%d\n", dummy);
		mutex_unlock(&cwmcu_lock);
		return dummy;
	}
		}
	else
		{
		sensor->client->addr = sensor->client->addr & I2C_MASK_FLAG | I2C_DMA_FLAG;
		sensor->client->timing = 400;	
		dummy = i2c_master_send(sensor->client, CWI2CDMABuf_pa, len);
		mutex_unlock(&cwmcu_lock);
		return dummy;
		}

	//printk(KERN_ERR "LENOVO00000 i2c write OK data[0]=%02x,data[1]=%02x\n", data[0],data[1]);
	mutex_unlock(&cwmcu_lock);
	return 0;
}

static int CWMCU_read_serial(u8 *data, int len)
{
	int dummy,i = 0;
	mutex_lock(&cwmcu_lock);
	if(len < 8)
	{
		sensor->client->addr = sensor->client->addr & I2C_MASK_FLAG;
		sensor->client->timing = 400;	
	dummy = i2c_master_recv(sensor->client, data, len);
	if (dummy < 0) {
		printk(KERN_ERR "i2c read error =%d\n", dummy);
		mutex_unlock(&cwmcu_lock);
		return dummy;
	}
		}
	else
		{
		sensor->client->addr = sensor->client->addr & I2C_MASK_FLAG | I2C_DMA_FLAG;
		//MSE_ERR("Sensor dma read timing is %x!\r\n", this_client->timing);
		sensor->client->timing = 400;	
		dummy = i2c_master_recv(sensor->client, CWI2CDMABuf_pa, len);
		
	    if(dummy < 0)
	    {
			mutex_unlock(&cwmcu_lock);
			return dummy;
		}

		for(i = 0; i < len; i++)
		{
			data[i] = CWI2CDMABuf_va[i];
		}
		}
	//printk(KERN_ERR "i2c read OK =%s,data[0]=%02x,ACK=%02x\n", __func__,data[0],ACK);
	mutex_unlock(&cwmcu_lock);
	return 0;
}

static int CWMCU_Set_Mcu_Mode(int mode)
{
	switch (sensor->mcu_mode) {
	case CW_NORMAL:
		sensor->mcu_mode = mode;
		break;
	case CW_SLEEP:
		sensor->mcu_mode = mode;
		break;
	case CW_NO_SLEEP:
		sensor->mcu_mode = mode;
		break;
	case CW_BOOT:
		sensor->mcu_mode = mode;
		break;
	default:
		return -EAGAIN;
	}
	return 0;
}

static void cwmcu_debuglog(void)
{
#if 1
	u8 data[40] = {0};
	s16 data_buff[3] = {0};
	int i = 0;

	if (CWMCU_i2c_read(sensor, CW_ERRORCOUNT, data, 1) >= 0) {
		data_buff[0] = (s16)data[0];
		/* printk(KERN_DEBUG "errorcount = %d\n",data_buff[0]); */
		for (i = 0; i < data_buff[0]; i++) {
			if (CWMCU_i2c_read(sensor, CW_ERRORLOG, data, 30) >= 0)
				printk(KERN_DEBUG "CW->%s\n", data);
		}
	}
#endif
}

static void cwmcu_powermode_switch(SWITCH_POWER_ID id, int onoff)
{
	if (onoff) {
		if (sensor->power_on_list == 0) {
			mt_set_gpio_out(GPIO_CW_MCU_WAKE_UP, onoff);
		}
		sensor->power_on_list |= ((uint32_t)(1) << id);
		usleep_range(400, 600);
	} else {
		sensor->power_on_list &= ~(1 << id);
		if (sensor->power_on_list == 0) {
			mt_set_gpio_out(GPIO_CW_MCU_WAKE_UP, onoff);
		}
	}
	//printk(KERN_ERR "--CWMCU--%s id = %d, onoff = %d\n", __func__, id, onoff);
}

/*
	Sensor get data and report event
	format byte[0] = id
	format byte[1] & byte[2] = time diff [L/H]
	format byte[3]~byte[8] = data X,Y,Z [L/H]
*/
static void cwmcu_batch_read(struct CWMCU_data *sensor)
{
	int i = 0;
	int event_count = 0;
	uint8_t data[20] = {0};
	uint8_t data_buff = 0;
	uint32_t data_event[4] = {0};

	/* read the count of batch queue */
	if (CWMCU_i2c_read(sensor, CW_BATCHCOUNT, data, 2) >= 0) {
		data_event[0] = ((u32)data[1] << 8) | (u32)data[0];
		event_count = data_event[0];
		printk(KERN_DEBUG "--CWMCU-- batch count %d\n", event_count);
	} else {
		printk(KERN_DEBUG "--CWMCU-- check batch count failed~!!\n");
	}

	for (i = 0; i < event_count; i++) {
		if (CWMCU_i2c_read(sensor, CW_BATCHEVENTDATA, data, 9) >= 0) {
			/* check if there are no data from queue */
			if (data[0] != CWMCU_NODATA) {
				if (data[0] == CW_META_DATA) {
					data_event[1] = ((u32)data[0] << 16) | ((u32)data[4] << 8) | (u32)data[3];
					printk(KERN_DEBUG "--CWMCU-- CW_META_DATA return flush event_id = %d complete~!!\n", data[3]);
					input_report_abs(sensor->input, CW_ABS_Z, data_event[1]);
					input_sync(sensor->input);
				} else if (data[0] == CW_MAGNETIC_UNCALIBRATED_BIAS) {
					data_buff = CW_MAGNETIC_UNCALIBRATED;
					data_event[1] = ((u32)data_buff << 16) | ((u32)data[4] << 8) | (u32)data[3];
					data_event[2] = ((u32)data_buff << 16) | ((u32)data[6] << 8) | (u32)data[5];
					data_event[3] = ((u32)data_buff << 16) | ((u32)data[8] << 8) | (u32)data[7];

					printk(KERN_DEBUG "--CWMCU-- Batch data: total count = %d, current count = %d, event_id = %d, data_x = %d, data_y = %d, data_z = %d\n"
									, event_count
									, i
									, data[0]
									, ((int16_t)(((u32)data[4] << 8) | (u32)data[3]))
									, ((int16_t)(((u32)data[6] << 8) | (u32)data[5]))
									, ((int16_t)(((u32)data[8] << 8) | (u32)data[7]))
									);
					input_report_abs(sensor->input, CW_ABS_X1, data_event[1]);
					input_report_abs(sensor->input, CW_ABS_Y1, data_event[2]);
					input_report_abs(sensor->input, CW_ABS_Z1, data_event[3]);
				} else if (data[0] == CW_GYROSCOPE_UNCALIBRATED_BIAS) {
					data_buff = CW_GYROSCOPE_UNCALIBRATED;
					data_event[1] = ((u32)data_buff << 16) | ((u32)data[4] << 8) | (u32)data[3];
					data_event[2] = ((u32)data_buff << 16) | ((u32)data[6] << 8) | (u32)data[5];
					data_event[3] = ((u32)data_buff << 16) | ((u32)data[8] << 8) | (u32)data[7];

					printk(KERN_DEBUG "--CWMCU-- Batch data: total count = %d, current count = %d, event_id = %d, data_x = %d, data_y = %d, data_z = %d\n"
									, event_count
									, i
									, data[0]
									, ((int16_t)(((u32)data[4] << 8) | (u32)data[3]))
									, ((int16_t)(((u32)data[6] << 8) | (u32)data[5]))
									, ((int16_t)(((u32)data[8] << 8) | (u32)data[7]))
									);
					input_report_abs(sensor->input, CW_ABS_X1, data_event[1]);
					input_report_abs(sensor->input, CW_ABS_Y1, data_event[2]);
					input_report_abs(sensor->input, CW_ABS_Z1, data_event[3]);
				} else {
					data_event[0] = ((u32)data[0] << 16) | ((u32)data[2] << 8) | (u32)data[1];
					data_event[1] = ((u32)data[0] << 16) | ((u32)data[4] << 8) | (u32)data[3];
					data_event[2] = ((u32)data[0] << 16) | ((u32)data[6] << 8) | (u32)data[5];
					data_event[3] = ((u32)data[0] << 16) | ((u32)data[8] << 8) | (u32)data[7];
					/*
					printk(KERN_DEBUG "--CWMCU-- Batch data: total count = %d, current count = %d, event_id = %d, data_x = %d, data_y = %d, data_z = %d\n"
									, event_count
									, i
									, data[0]
									, ((int16_t)(((u32)data[4] << 8) | (u32)data[3]))
									, ((int16_t)(((u32)data[6] << 8) | (u32)data[5]))
									, ((int16_t)(((u32)data[8] << 8) | (u32)data[7]))
									);
					*/
					/* check flush event */
					input_report_abs(sensor->input, CW_ABS_X, data_event[1]);
					input_report_abs(sensor->input, CW_ABS_Y, data_event[2]);
					input_report_abs(sensor->input, CW_ABS_Z, data_event[3]);
					input_report_abs(sensor->input, CW_ABS_TIMEDIFF, data_event[0]);
					input_sync(sensor->input);
				}
			}
		}
	}
}

static void cwmcu_gesture_read(struct CWMCU_data *sensor)
{
	uint8_t data[2] = {0};
	uint32_t data_event;
	int data_count = 0;
	int i = 0;

	if (CWMCU_i2c_read(sensor, CW_READ_GESTURE_EVENT_COUNT, data, 1) >= 0) {
		data_count = data[0];
		for (i = 0; i < data_count; i++) {
			/* read 2byte */
			if (CWMCU_i2c_read(sensor, CW_READ_GESTURE_EVENT_DATA, data, 2) >= 0) {
				if (data[0] != CWMCU_NODATA) {
					data_event = ((u32)data[0] << 16) | (((u32)data[1]));
					printk(KERN_DEBUG "--CWMCU--Normal gesture %d data -> x = %d\n"
							, data[0]
							, data[1]
							);
					input_report_abs(sensor->input, CW_ABS_X, data_event);
					input_sync(sensor->input);
				}
			} else {
				printk(KERN_DEBUG "--CWMCU-- read gesture failed~!!\n");
			}
		}
	}
}

static void cwmcu_send_flush(int id)
{
	uint32_t flush_data = 0;

	flush_data = ((u32)CW_META_DATA << 16) | id;
	printk(KERN_DEBUG "--CWMCU-- flush sensor: %d auto return~!!\n", id);
	input_report_abs(sensor->input, CW_ABS_Z, flush_data);
	input_sync(sensor->input);
	/* reset flush event for ABS_Z*/
	input_report_abs(sensor->input, CW_ABS_Z, 0);
	input_sync(sensor->input);
}
#if 0
static void cwmcu_check_sensor_update(void)
{
	int id = 0;
	int64_t tvusec, temp = 0;
	unsigned int tvsec;

	do_gettimeofday(&sensor->now);
	tvsec = sensor->now.tv_sec;
	tvusec = sensor->now.tv_usec;

	temp = (int64_t)(tvsec * 1000000LL) + tvusec;

	/* printk(KERN_DEBUG "--CWMCU-- time(u) = %llu, tv_sec = %u, v_usec = %llu\n",temp, tvsec, tvusec); */

	for (id = 0; id < CW_SENSORS_ID_END; id++) {

		if( sensor->enabled_list & (1<<id) && (sensor->sensor_timeout[id] == 0)) {
			/* printk(KERN_DEBUG "--CWMCU-- id = %d\n", id); */
			sensor->time_diff[id] = temp - sensor->sensors_time[id];
			if (sensor->time_diff[id] >= (sensor->report_period[id] * 1000)) {
				printk(KERN_DEBUG "--CWMCU-- sensor->time_diff[id] = %lld, sensor->report_period[id] = %lld\n"
							, sensor->time_diff[id], sensor->report_period[id] * 1000);
				sensor->update_list |= 1<<id;
				sensor->sensors_time[id] = temp;
			} else {
				sensor->update_list &= ~(1<<id);
			}
		} else {
			sensor->update_list &= ~(1<<id);
		}
	}
	printk(KERN_DEBUG "--CWMCU-- sensor->update_list = %d\n", sensor->update_list);
}
#endif
static int CWMCU_read(struct CWMCU_data *sensor)
{
	int id_check = 0;
	uint8_t data[20] = {0};
	uint32_t data_event[7] = {0};

	if (sensor->mcu_mode == CW_BOOT) {
		/* it will not get data if status is bootloader mode */
		return 0;
	}

	/* cwmcu_check_sensor_update(); */

	if (sensor->update_list) {
		for (id_check = 0; id_check < CW_SENSORS_ID_END; id_check++) {
			if ((sensor->update_list & (1<<id_check)) && (sensor->sensor_timeout[id_check] == 0)) {
					switch (id_check) {
					case CW_ACCELERATION:
					case CW_MAGNETIC:
					case CW_GYRO:
					//case CW_LIGHT:
					case CW_PROXIMITY:
					case CW_PRESSURE:
					case CW_ORIENTATION:
					case CW_ROTATIONVECTOR:
					case CW_LINEARACCELERATION:
					case CW_GRAVITY:
					case CW_AIRMOUSE:
					case CW_GAME_ROTATION_VECTOR:
					case CW_GEOMAGNETIC_ROTATION_VECTOR:
					case CW_TILT:
					case CW_PDR:
							/* read 6byte */
							if (CWMCU_i2c_read(sensor, CWMCU_I2C_SENSORS_REG_START+id_check, data, 6) >= 0) {
									data_event[0] = ((u32)id_check << 16) | (((u32)data[1] << 8) | (u32)data[0]);
									data_event[1] = ((u32)id_check << 16) | (((u32)data[3] << 8) | (u32)data[2]);
									data_event[2] = ((u32)id_check << 16) | (((u32)data[5] << 8) | (u32)data[4]);

									printk(KERN_DEBUG "--CWMCU--Normal %d data -> x = %d, y = %d, z = %d\n"
												, id_check
												, (int16_t)((u32)data[1] << 8) | (u32)data[0]
												, (int16_t)((u32)data[3] << 8) | (u32)data[2]
												, (int16_t)((u32)data[5] << 8) | (u32)data[4]
												);
									if (id_check == CW_MAGNETIC || id_check == CW_ORIENTATION) {
										if (CWMCU_i2c_read(sensor, CW_ACCURACY, data, 1) >= 0) {
											data_event[6] = ((u32)id_check << 16) | (u32)data[0];
										}
										printk(KERN_DEBUG "--CWMCU--MAG ACCURACY = %d\n", data[0]);
										input_report_abs(sensor->input, CW_ABS_X, data_event[0]);
										input_report_abs(sensor->input, CW_ABS_Y, data_event[1]);
										input_report_abs(sensor->input, CW_ABS_Z, data_event[2]);
										input_report_abs(sensor->input, CW_ABS_ACCURACY, data_event[6]);
										input_sync(sensor->input);
									} else {
										input_report_abs(sensor->input, CW_ABS_X, data_event[0]);
										input_report_abs(sensor->input, CW_ABS_Y, data_event[1]);
										input_report_abs(sensor->input, CW_ABS_Z, data_event[2]);
										input_sync(sensor->input);
									}
							} else {
								printk(KERN_DEBUG "--CWMCU-- CWMCU_i2c_read error 0x%x~!!!\n", CWMCU_I2C_SENSORS_REG_START+id_check);
							}
							break;
					case CW_LIGHT:
							/* read 6byte */
							if (CWMCU_i2c_read(sensor, CWMCU_I2C_SENSORS_REG_START+id_check, data, 6) >= 0) {
									data_event[0] = ((u32)id_check << 16) | (((u32)data[1] << 8) | (u32)data[0]);
									data_event[1] = ((u32)id_check << 16) | (((u32)data[3] << 8) | (u32)data[2]);
									data_event[2] = ((u32)id_check << 16) | (((u32)data[5] << 8) | (u32)data[4]);

									printk(KERN_DEBUG "--CWMCU--Normal %d data -> x = %d, y = %d, z = %d\n"
												, id_check
												, (int16_t)((u32)data[1] << 8) | (u32)data[0]
												, (int16_t)((u32)data[3] << 8) | (u32)data[2]
												, (int16_t)((u32)data[5] << 8) | (u32)data[4]
												);
									input_report_abs(sensor->input, CW_ABS_X, data_event[1]);
									input_report_abs(sensor->input, CW_ABS_Y, data_event[0]);
									input_report_abs(sensor->input, CW_ABS_Z, data_event[2]);
									input_sync(sensor->input);
									/* reset x,y,z */
									input_report_abs(sensor->input, CW_ABS_X, 0xFF0000);
									input_report_abs(sensor->input, CW_ABS_Y, 0xFF0000);
									input_report_abs(sensor->input, CW_ABS_Z, 0xFF0000);
									input_report_abs(sensor->input, CW_ABS_ACCURACY, 0xFF0000);
									input_sync(sensor->input);
							} else {
								printk(KERN_DEBUG "--CWMCU-- CWMCU_i2c_read error 0x%x~!!!\n", CWMCU_I2C_SENSORS_REG_START+id_check);
							}
							break;
					case CW_RGB:
							/* read 8byte */
							if (CWMCU_i2c_read(sensor, CWMCU_I2C_SENSORS_REG_START+id_check, data, 8) >= 0) {
									data_event[0] = ((u32)id_check << 16) | (((u32)data[1] << 8) | (u32)data[0]);
									data_event[1] = ((u32)id_check << 16) | (((u32)data[3] << 8) | (u32)data[2]);
									data_event[2] = ((u32)id_check << 16) | (((u32)data[5] << 8) | (u32)data[4]);
									data_event[3] = ((u32)id_check << 16) | (((u32)data[7] << 8) | (u32)data[6]);

									printk(KERN_DEBUG "--CWMCU--Normal %d data -> x = %d, y = %d, z = %d, ct = %d\n"
												, id_check
												, (int16_t)((u32)data[1] << 8) | (u32)data[0]
												, (int16_t)((u32)data[3] << 8) | (u32)data[2]
												, (int16_t)((u32)data[5] << 8) | (u32)data[4]
												, (int16_t)((u32)data[7] << 8) | (u32)data[6]
												);
									input_report_abs(sensor->input, CW_ABS_X, data_event[0]);
									input_report_abs(sensor->input, CW_ABS_Y, data_event[1]);
									input_report_abs(sensor->input, CW_ABS_Z, data_event[2]);
									input_report_abs(sensor->input, CW_ABS_X1, data_event[3]);
									input_sync(sensor->input);
							}
							break;
					case CW_MAGNETIC_UNCALIBRATED:
					case CW_GYROSCOPE_UNCALIBRATED:
							/* read 12byte */
							if (CWMCU_i2c_read(sensor, CWMCU_I2C_SENSORS_REG_START+id_check, data, 12) >= 0) {
									data_event[0] = ((u32)id_check << 16) | (((u32)data[1] << 8) | (u32)data[0]);
									data_event[1] = ((u32)id_check << 16) | (((u32)data[3] << 8) | (u32)data[2]);
									data_event[2] = ((u32)id_check << 16) | (((u32)data[5] << 8) | (u32)data[4]);
									data_event[3] = ((u32)id_check << 16) | (((u32)data[7] << 8) | (u32)data[6]);
									data_event[4] = ((u32)id_check << 16) | (((u32)data[9] << 8) | (u32)data[8]);
									data_event[5] = ((u32)id_check << 16) | (((u32)data[11] << 8) | (u32)data[10]);

									printk(KERN_DEBUG "--CWMCU--Normal %d data -> x = %d, y = %d, z = %d, x_bios = %d, y_bios = %d, z_bios = %d,\n"
												, id_check
												, (int16_t)((u32)data[1] << 8) | (u32)data[0]
												, (int16_t)((u32)data[3] << 8) | (u32)data[2]
												, (int16_t)((u32)data[5] << 8) | (u32)data[4]
												, (int16_t)((u32)data[7] << 8) | (u32)data[6]
												, (int16_t)((u32)data[9] << 8) | (u32)data[8]
												, (int16_t)((u32)data[11] << 8) | (u32)data[10]
												);
									if (CWMCU_i2c_read(sensor, CW_ACCURACY, data, 1) >= 0) {
											data_event[6] = ((u32)id_check << 16) | (u32)data[0];
										}
									input_report_abs(sensor->input, CW_ABS_X, data_event[0]);
									input_report_abs(sensor->input, CW_ABS_Y, data_event[1]);
									input_report_abs(sensor->input, CW_ABS_Z, data_event[2]);
									input_report_abs(sensor->input, CW_ABS_X1, data_event[3]);
									input_report_abs(sensor->input, CW_ABS_Y1, data_event[4]);
									input_report_abs(sensor->input, CW_ABS_Z1, data_event[5]);
									input_report_abs(sensor->input, CW_ABS_ACCURACY, data_event[6]);
									input_sync(sensor->input);
							}
							break;
					case CW_PEDOMETER:
								/* read 6byte */
								if (CWMCU_i2c_read(sensor, CWMCU_I2C_SENSORS_REG_START+id_check, data, 6) >= 0) {
									data_event[0] = ((u32)id_check << 16) | (((u32)data[1] << 8) | (u32)data[0]);
									data_event[1] = ((u32)id_check << 16) | (((u32)data[3] << 8) | (u32)data[2]);
									data_event[2] = ((u32)id_check << 16) | (((u32)data[5] << 8) | (u32)data[4]);

									printk(KERN_DEBUG "--CWMCU--Normal %d data -> x = %d, y = %d, z = %d\n"
												, id_check
												, (int16_t)((u32)data[1] << 8) | (u32)data[0]
												, (int16_t)((u32)data[3] << 8) | (u32)data[2]
												, (int16_t)((u32)data[5] << 8) | (u32)data[4]
												);
									if (data_event[0] != 0) {
										input_report_abs(sensor->input, CW_ABS_X, data_event[0]);
										input_report_abs(sensor->input, CW_ABS_Y, data_event[1]);
										input_report_abs(sensor->input, CW_ABS_Z, data_event[2]);
										input_sync(sensor->input);
									}
								} else {
									printk(KERN_DEBUG "--CWMCU-- CWMCU_i2c_read error 0x%x~!!!\n", CWMCU_I2C_SENSORS_REG_START+id_check);
								}
								break;
					default:
								break;
					}
				}
			}
		}

#ifndef CWMCU_INTERRUPT
		sensor->debug_count++;
		/* show debug log if there are error form mcu */
		if (sensor->debug_count == 20) {
			cwmcu_powermode_switch(SWITCH_POWER_INTERRUPT, 1);
			cwmcu_debuglog();
			cwmcu_batch_read(sensor);
			/* read gesture event */
			cwmcu_gesture_read(sensor);

			cwmcu_powermode_switch(SWITCH_POWER_INTERRUPT, 0);
			sensor->debug_count = 0;
		}
#endif
	return 0;
}

/*==========sysfs node=====================*/

static int active_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int enabled = 0;
	int sensors_id = 0;
	int error_msg = 0;
	u8 data = 0;
	u8 i = 0,j = 0;
	uint8_t delay_ms = 0;

	if (sensor->mcu_mode == CW_BOOT) {
		return count;
	}

	sscanf(buf, "%d %d\n", &sensors_id, &enabled);

	sensor->enabled_list &= ~(1<<sensors_id);
	sensor->enabled_list |= ((uint32_t)enabled)<<sensors_id;

	/* clean timeout value if sensor turn off */
	if (enabled == 0) {
		sensor->sensor_timeout[sensors_id] = 0;
		sensor->sensors_time[sensors_id] = 0;
	} else {
		do_gettimeofday(&sensor->now);
		sensor->sensors_time[sensors_id] = (sensor->now.tv_sec * 1000000LL) + sensor->now.tv_usec;
	}

	i = sensors_id / 8;
	data = (u8)(sensor->enabled_list>>(i*8));

	if ((sensor->pwrmod != 1) || 
		(sensors_id == CW_PROXIMITY) ||
		(sensors_id == CW_PROXIMITY_SCREEN_ON) ||
		(sensors_id == CW_SCREEN_ON)) {

		for (j=0;j<10;j++) {
			cwmcu_powermode_switch(SWITCH_POWER_ENABLE, 1);

			error_msg += CWMCU_i2c_write(sensor, CW_ENABLE_REG+i, &data, 1);

			printk(KERN_DEBUG "--CWMCU--1 data =%d, i = %d, sensors_id=%d enable=%d  enable_list=%d\n", data, i, sensors_id, enabled, sensor->enabled_list);

			delay_ms = (uint8_t)sensor->report_period[sensors_id];

			error_msg += CWMCU_i2c_write(sensor, CW_DELAY_ACC+sensors_id, &delay_ms, 1);

			cwmcu_powermode_switch(SWITCH_POWER_ENABLE, 0);
			
			if (error_msg) {
				msleep(10);
				error_msg = 0;
				printk(KERN_DEBUG "--CWMCU--1 active_set retry=%d\n", j);
			}else
				break;
		}
	}
	return count;
}

static int active_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, sizeof(sensor->enabled_list), "%u\n", sensor->enabled_list);
}

static int interval_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, sizeof(CWMCU_POLL_INTERVAL), "%d\n", CWMCU_POLL_INTERVAL);
}

static int interval_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int val = 0;
	int sensors_id = 0;
	uint8_t delay_ms = 0;
	int error_msg = 0;
	u8 i = 0;

	if (sensor->mcu_mode == CW_BOOT) {
		return count;
	}

	sscanf(buf, "%d %d\n", &sensors_id , &val);
	if (val < CWMCU_POLL_MIN)
		val = CWMCU_POLL_MIN;

	sensor->report_period[sensors_id] = val;

	delay_ms = (uint8_t)val;
	for (i=0;i<10;i++) {

		cwmcu_powermode_switch(SWITCH_POWER_DELAY, 1);
		error_msg = CWMCU_i2c_write(sensor, CW_DELAY_ACC+sensors_id, &delay_ms, 1);
		cwmcu_powermode_switch(SWITCH_POWER_DELAY, 0);
		if (error_msg) {
			msleep(10);
			error_msg = 0;
			printk(KERN_DEBUG "--CWMCU-- interval_set retry=%d\n", i);
		}else
			break;
	}

	printk(KERN_DEBUG "--CWMCU-- sensors_id=%d delay_ms=%d\n", sensors_id, delay_ms);
	return count;
}

/*
static int poll_show(struct device *dev, struct device_attribute *attr, char *buf){

	//printk(KERN_DEBUG "--CWMCU-- %s in\n", __func__);

	//return  CWMCU_read(buf);
	return 0;
}
*/

static int batch_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int sensors_id = 0;
	int delay_ms = 0;
	int64_t timeout = 0;
	int batch_mode = -1;
	uint8_t data[5] = {0};
	int error_msg = 0;
	u8 i = 0;

	if (sensor->mcu_mode == CW_BOOT) {
		return count;
	}

	printk(KERN_DEBUG "--CWMCU-- %s in~!!\n", __func__);

	sscanf(buf, "%d %d %d %lld\n", &sensors_id, &batch_mode, &delay_ms, &timeout);

	printk(KERN_DEBUG "--CWMCU-- sensors_id = %d, timeout = %lld\n", sensors_id, timeout);

	sensor->sensor_timeout[sensors_id] = timeout;

	sensor->report_period[sensors_id] = delay_ms;

	data[0] = (uint8_t)sensors_id;
	data[1] = (uint8_t)(timeout);
	data[2] = (uint8_t)(timeout >> 8);
	data[3] = (uint8_t)(timeout >> 16);
	data[4] = (uint8_t)(timeout >> 24);

	if ((sensor->pwrmod != 1) || 
		(sensors_id == CW_PROXIMITY) ||
		(sensors_id == CW_PROXIMITY_SCREEN_ON) ||
		(sensors_id == CW_SCREEN_ON)) {

		for (i=0;i<10;i++) {
			cwmcu_powermode_switch(SWITCH_POWER_BATCH, 1);
			error_msg += CWMCU_write_i2c_block(sensor, CW_BATCHTIMEOUT, data, 5);
			cwmcu_powermode_switch(SWITCH_POWER_BATCH, 0);
			if (error_msg) {
				msleep(10);
				error_msg = 0;
				printk(KERN_DEBUG "--CWMCU-- batch_set retry=%d\n", i);
			}else
				break;
		}
	}
	printk(KERN_DEBUG "--CWMCU-- sensors_id = %d, current_timeout = %lld, delay_ms = %d\n", sensors_id, timeout, delay_ms);

	return count;
}

static int batch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, 255, "sensor->batched_list = %d, sensor->current_timeout = %lld\n"
					,sensor->batched_list, sensor->current_timeout);
}

static int flush_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int sensors_id = 0;
	int error_msg = 0;
	uint8_t data = 0;
	u8 i;

	if (sensor->mcu_mode == CW_BOOT) {
		return count;
	}

	printk(KERN_DEBUG "--CWMCU-- %s in\n", __func__);

	sscanf(buf, "%d\n", &sensors_id);
	data = (uint8_t)sensors_id;

	printk(KERN_DEBUG "--CWMCU-- flush sensors_id = %d~!!\n", sensors_id);

	/* check flush event */
	if (sensor->current_timeout == 0) {
		cwmcu_send_flush(sensors_id);
	} else {
		for (i=0;i<10;i++) {

			cwmcu_powermode_switch(SWITCH_POWER_BATCH, 1);

			error_msg = CWMCU_i2c_write(sensor, CW_BATCHFLUSH, &data, 1);

			if (error_msg < 0)
				printk(KERN_DEBUG "--CWMCU-- flush i2c error~!!\n");
			cwmcu_powermode_switch(SWITCH_POWER_BATCH, 0);
			if (error_msg) {
				msleep(10);
				error_msg = 0;
				printk(KERN_DEBUG "--CWMCU-- flush_set retry=%d\n", i);
			}else
				break;
		}

	}
	return count;
}

static int flush_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk(KERN_DEBUG "--CWMCU-- %s in\n", __func__);
	return  0;
}

#if 1
static int CWMCU_Erase_Mcu_Memory(void)
{
	/* page should be 1~N */
	uint8_t send[300];
	uint8_t received[10];
	uint8_t XOR = 0;
	uint8_t page;
	uint16_t i = 0;
	page = 128;

	send[0] = 0x44;
	send[1] = 0xBB;
	if (CWMCU_write_serial((uint8_t *)send, 2) < 0) {
		return -EAGAIN;
		}
	if (CWMCU_read_serial((uint8_t *)received, 1) < 0) {
		return -EAGAIN;
		}
	if (received[0] != ACK) {
		return -EAGAIN;
		}

	send[0] = (uint8_t) ((page-1)>>8);
	send[1] = (uint8_t) (page-1);
	send[2] = send[0] ^ send[1];
	if (CWMCU_write_serial((uint8_t *)send, 3) < 0) {
		return -EAGAIN;
		}
	if (CWMCU_read_serial((uint8_t *)received, 1) < 0) {
		return -EAGAIN;
		}
	if (received[0] != ACK) {
		return -EAGAIN;
		}

	for (i = 0; i < page; i++) {
		send[2*i] = (uint8_t)(i>>8);
		send[(2*i)+1] = (uint8_t)i;
		XOR = XOR ^ send[2*i] ^ send[(2*i)+1];
	}
	send[(2*page)] = XOR;
	if (CWMCU_write_serial((uint8_t *)send, ((2*page)+1)) < 0) {
		return -EAGAIN;
		}
	return 0;

}
/*
static int CWMCU_Free_Run(void){
	uint8_t send[10];
	uint8_t received[10];

	send[0] = 0x21;
	send[1] = 0xDE;
	if(CWMCU_write_serial((uint8_t*)send, 2)<0){return -1;}
	if(CWMCU_read_serial(received, 1)<0){return -1;}
	if(received[0] !=ACK){return -1;}

	send[0] = 0x08;
	send[1] = 0x00;
	send[2] = 0x00;
	send[3] = 0x00;
	send[4] = send[0]^send[1]^send[2]^send[3];
	if(CWMCU_write_serial((uint8_t*)send, 5)<0){return -1;}
#if 0
	if(CWMCU_read_serial((uint8_t*)received, 1)<0){return -1;}
	if(received[0] !=ACK){return -1;}
#endif
	return 0;
}
*/
static int CWMCU_Write_Mcu_Memory(const char *buf)
{
	uint8_t WriteMemoryCommand[2];
	uint8_t data[300];
	uint8_t received[10];
	uint8_t XOR = 0;
	uint16_t i = 0;
	WriteMemoryCommand[0] = 0x31;
	WriteMemoryCommand[1] = 0xCE;
	if (CWMCU_write_serial((uint8_t *)WriteMemoryCommand, 2) < 0) {
		return -EAGAIN;
		}
	if (CWMCU_read_serial((uint8_t *)received, 1) < 0) {
		return -EAGAIN;
		}
	if (received[0] != ACK) {
		return -EAGAIN;
		}

	/* Set Address + Checksum */
	data[0] = (uint8_t) (sensor->addr >> 24);
	data[1] = (uint8_t) (sensor->addr >> 16);
	data[2] = (uint8_t) (sensor->addr >> 8);
	data[3] = (uint8_t) sensor->addr;
	data[4] = data[0] ^ data[1] ^ data[2] ^ data[3];
	if (CWMCU_write_serial((uint8_t *)data, 5) < 0) {
		return -EAGAIN;
		}
	if (CWMCU_read_serial((uint8_t *)received, 1) < 0) {
		return -EAGAIN;
		}
	if (received[0] != ACK) {
		return -EAGAIN;
		}

	/* send data */
	data[0] = sensor->len - 1;
	XOR = sensor->len - 1;
	for (i = 0; i < sensor->len; i++) {
		data[i+1] = buf[i];
		XOR ^= buf[i];
	}
	data[sensor->len+1] = XOR;

	if (CWMCU_write_serial((uint8_t *)data, (sensor->len + 2)) < 0) {
		return -EAGAIN;
		}
	return 0;
}

#else

static int CWMCU_Erase_Mcu_Memory(void)
{
	return 0;
}
static int CWMCU_Free_Run(void)
{
	return 0;
}
static int CWMCU_Write_Mcu_Memory(const char *buf)
{
	return 0;
}

#endif

static int cwmcu_reinit(void)
{
	int id;
	int part;
	int error_msg = 0;
	uint8_t delay_ms = 0;
	u8 list;
	uint8_t data[5] = {0};

	printk(KERN_DEBUG "--CWMCU-- %s in, sensor->enabled_list = %d\n", __func__, sensor->enabled_list);

	for (id = 0; id < CW_SENSORS_ID_END; id++) {
		printk(KERN_DEBUG "--CWMCU--%d\n", id);
		if (sensor->enabled_list & (1<<id)) {
			part = id / 8;
			list = (u8)(sensor->enabled_list>>(part*8));

			data[0] = (uint8_t)id;
			data[1] = (uint8_t)(sensor->sensor_timeout[id]);
			data[2] = (uint8_t)(sensor->sensor_timeout[id] >> 8);
			data[3] = (uint8_t)(sensor->sensor_timeout[id] >> 16);
			data[4] = (uint8_t)(sensor->sensor_timeout[id] >> 24);
			delay_ms = (uint8_t)sensor->report_period[id];

			error_msg += CWMCU_i2c_write(sensor, CW_ENABLE_REG+part, &list, 1);
			CWMCU_write_i2c_block(sensor, CW_BATCHTIMEOUT, data, 5);
			CWMCU_i2c_write(sensor, CW_DELAY_ACC+id, &delay_ms, 1);

			printk(KERN_DEBUG "--CWMCU-- sensors_id=%d , enable_list=%d, sensor->sensor_timeout= %lld\n", id, sensor->enabled_list, sensor->sensor_timeout[id]);
		}
	}
	return 0;
}

static int set_firmware_update_cmd(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	u8 data[40] = {0};
	s16 data_buff[3] = {0};
	s16  _bData[8];
	s16  _bData2[9];
	s16  m_hdata[3];
	s16  m_asa[3];
	u8 test = 0x01;

	sscanf(buf, "%d %d %d\n", &sensor->cmd, &sensor->addr, &sensor->len);
	printk(KERN_DEBUG "CWMCU cmd=%d addr=%d len=%d\n", sensor->cmd, sensor->addr, sensor->len);

	cwmcu_powermode_switch(SWITCH_POWER_ENABLE, 1);

	switch (sensor->cmd) {
	case CHANGE_TO_BOOTLOADER_MODE:
			printk(KERN_DEBUG "CWMCU CHANGE_TO_BOOTLOADER_MODE\n");
			/* boot enable : put high , reset: put low */
			#if 1
			//mt_set_gpio_dir(GPIO_AST_CS_PIN, 1);
			mt_set_gpio_out(GPIO_AST_RST_PIN, 1);
			msleep(10);
			mt_set_gpio_out(GPIO_AST_CS_PIN, 1);
			msleep(10);
			mt_set_gpio_out(GPIO_AST_CS_PIN, 0);
			msleep(10);
			mt_set_gpio_out(GPIO_AST_CS_PIN, 1);
			msleep(200);
                    #endif
			sensor->mcu_mode = CW_BOOT;
			sensor->mcu_slave_addr = sensor->client->addr;
			sensor->client->addr = 0x72 >> 1;
			break;

	case CHANGE_TO_NORMAL_MODE:
			printk(KERN_DEBUG "CWMCU CHANGE_TO_NORMAL_MODE\n");

			sensor->firmwave_update_status = 1;
			sensor->client->addr = 0x74 >> 1;

			/* boot low  reset high */
			#if 1
			mt_set_gpio_out(GPIO_AST_RST_PIN, 0);
			mt_set_gpio_out(GPIO_AST_CS_PIN, 1);
			msleep(10);
			mt_set_gpio_out(GPIO_AST_CS_PIN, 0);
			msleep(10);
			mt_set_gpio_out(GPIO_AST_CS_PIN, 1);
			msleep(200);

			//mt_set_gpio_dir(GPIO_AST_CS_PIN,0);
                     #endif
			sensor->mcu_mode = CW_NORMAL;
			break;

	case ERASE_MCU_MEMORY:
			printk(KERN_DEBUG "CWMCU ERASE_MCU_MEMORY\n");
			sensor->firmwave_update_status = 1;
			sensor->firmwave_update_status = CWMCU_Erase_Mcu_Memory();
			break;

	case WRITE_MCU_MEMORY:
			printk(KERN_DEBUG "CWMCU Set Addr=%d\tlen=%d\n", sensor->addr, sensor->len);
			break;

	case MCU_GO:
			printk(KERN_DEBUG "CWMCU MCU_GO\n");
			break;

	case CHECK_FIRMWAVE_VERSION:
			if (CWMCU_i2c_read(sensor, CW_FWVERSION, data, 1) >= 0) {
				printk(KERN_DEBUG "CHECK_FIRMWAVE_VERSION %d\n", (int)data[0]);
			}
			break;

	case CHECK_ACC_DATA:
			printk(KERN_DEBUG "CWMCU CHECK_ACC_DATA\n");
			if (CWMCU_i2c_read(sensor, CW_READ_ACCELERATION, data, 6) >= 0) {
				data_buff[0] = (s16)(((u16)data[1] << 8) | (u16)data[0]);
				data_buff[1] = (s16)(((u16)data[3] << 8) | (u16)data[2]);
				data_buff[2] = (s16)(((u16)data[5] << 8) | (u16)data[4]);

				printk(KERN_DEBUG "x = %d, y = %d, z = %d\n",
					data_buff[0], data_buff[1], data_buff[2]);
			}
			break;

	case CHECK_MAG_DATA:
			printk(KERN_DEBUG "CWMCU CHECK_MAG_DATA\n");
			if (CWMCU_i2c_read(sensor, CW_READ_MAGNETIC, data, 6) >= 0) {
				data_buff[0] = (s16)(((u16)data[1] << 8) | (u16)data[0]);
				data_buff[1] = (s16)(((u16)data[3] << 8) | (u16)data[2]);
				data_buff[2] = (s16)(((u16)data[5] << 8) | (u16)data[4]);

				printk(KERN_DEBUG "x = %d, y = %d, z = %d\n",
					data_buff[0], data_buff[1], data_buff[2]);
			}
			break;

	case CHECK_GYRO_DATA:
			printk(KERN_DEBUG "CWMCU CHECK_GYRO_DATA\n");
			if (CWMCU_i2c_read(sensor, CW_READ_GYRO, data, 6) >= 0) {
				data_buff[0] = (s16)(((u16)data[1] << 8) | (u16)data[0]);
				data_buff[1] = (s16)(((u16)data[3] << 8) | (u16)data[2]);
				data_buff[2] = (s16)(((u16)data[5] << 8) | (u16)data[4]);

				printk(KERN_DEBUG "x = %d, y = %d, z = %d\n",
					data_buff[0], data_buff[1], data_buff[2]);
			}
			break;
	case CHECK_GAME_R_DATA:
			printk(KERN_DEBUG "CWMCU CW_GAME_ROTATION_VECTOR\n");
			if (CWMCU_i2c_read(sensor, CW_READ_GAME_ROTATION_VECTOR, data, 6) >= 0) {
				data_buff[0] = (s16)(((u16)data[1] << 8) | (u16)data[0]);
				data_buff[1] = (s16)(((u16)data[3] << 8) | (u16)data[2]);
				data_buff[2] = (s16)(((u16)data[5] << 8) | (u16)data[4]);

				printk(KERN_DEBUG "x = %d, y = %d, z = %d\n",
					data_buff[0], data_buff[1], data_buff[2]);
			}
			break;
	case CHECK_GEMO_R_DATA:
			printk(KERN_DEBUG "CWMCU CW_GEOMAGNETIC_ROTATION_VECTOR\n");
			if (CWMCU_i2c_read(sensor, CW_READ_GEOMAGNETIC_ROTATION_VECTOR, data, 6) >= 0) {
				data_buff[0] = (s16)(((u16)data[1] << 8) | (u16)data[0]);
				data_buff[1] = (s16)(((u16)data[3] << 8) | (u16)data[2]);
				data_buff[2] = (s16)(((u16)data[5] << 8) | (u16)data[4]);

				printk(KERN_DEBUG "x = %d, y = %d, z = %d\n",
					data_buff[0], data_buff[1], data_buff[2]);
			}
			break;
	case CHECK_UNCALMAG_DATA:
			printk(KERN_DEBUG "CWMCU CW_MAGNETIC_UNCALIBRATED\n");
			if (CWMCU_i2c_read(sensor, CW_READ_MAGNETIC_UNCALIBRATED, data, 6) >= 0) {
				data_buff[0] = (s16)(((u16)data[1] << 8) | (u16)data[0]);
				data_buff[1] = (s16)(((u16)data[3] << 8) | (u16)data[2]);
				data_buff[2] = (s16)(((u16)data[5] << 8) | (u16)data[4]);

				printk(KERN_DEBUG "x = %d, y = %d, z = %d\n",
					data_buff[0], data_buff[1], data_buff[2]);
			}
			break;
	case CHECK_UNCALGYRO_DATA:
			printk(KERN_DEBUG "CWMCU CW_GYROSCOPE_UNCALIBRATED\n");
			if (CWMCU_i2c_read(sensor, CW_READ_GYROSCOPE_UNCALIBRATED, data, 6) >= 0) {
				data_buff[0] = (s16)(((u16)data[1] << 8) | (u16)data[0]);
				data_buff[1] = (s16)(((u16)data[3] << 8) | (u16)data[2]);
				data_buff[2] = (s16)(((u16)data[5] << 8) | (u16)data[4]);

				printk(KERN_DEBUG "x = %d, y = %d, z = %d\n",
					data_buff[0], data_buff[1], data_buff[2]);
			}
			break;

	case CHECK_HWID:
			printk(KERN_DEBUG "CWMCU CHECK_HWID\n");
			break;

	case CHECK_INFO:
			printk(KERN_DEBUG "CWMCU check info\n");
			cwmcu_debuglog();
			break;

	case CHECK_MAG1_INFO:
			if (CWMCU_i2c_read(sensor, CW_MAGINFO, data, 30) >= 0) {
				memcpy(_bData,&data[0],sizeof(_bData));
				memcpy(m_hdata,&data[16],sizeof(m_hdata));
				memcpy(m_asa,&data[22],sizeof(m_asa));
			printk("Disp_AKMDEBUG_1:%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d \n",
                                _bData[0],
                                (s16)(((uint16_t)_bData[2])<<8|_bData[1]),
                                (s16)(((uint16_t)_bData[4])<<8|_bData[3]),
                                (s16)(((uint16_t)_bData[6])<<8|_bData[5]),
                                _bData[7],
                                m_hdata[0], m_hdata[1], m_hdata[2],
                                m_asa[0], m_asa[1], m_asa[2]);
			}
			break;

	case CHECK_MAG2_INFO:
			if (CWMCU_i2c_read(sensor, CW_MAGINFO, data, 30) >= 0) {
				memcpy(_bData2,&data[0],sizeof(_bData));
				memcpy(m_hdata,&data[18],sizeof(m_hdata));
				memcpy(m_asa,&data[24],sizeof(m_asa));
			printk("Disp_AKMDEBUG_2:%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d \n",
                                _bData2[0],
                                (s16)(((uint16_t)_bData2[2])<<8|_bData2[1]),
                                (s16)(((uint16_t)_bData2[4])<<8|_bData2[3]),
                                (s16)(((uint16_t)_bData2[6])<<8|_bData2[5]),
                                _bData2[8],
                                m_hdata[0], m_hdata[1], m_hdata[2],
                                m_asa[0], m_asa[1], m_asa[2]);
			}
			break;
	case LED_CTRL:
			if(sensor->mcu_mode != CW_BOOT){
				printk(KERN_DEBUG "CWMCU LED control \n");
				test = (u8)sensor->len;
				CWMCU_i2c_write(sensor, CW_LED_CTRL, &test, 1);
				printk(KERN_DEBUG "CWMCU LED control end\n");
			 } else {
				printk(KERN_DEBUG "CWMCU mcu in boot mode, return\n");
			 }
			break;
	case TEST:
			printk(KERN_DEBUG "CWMCU watch dog timeout\n");
			test = 0x02;
			CWMCU_i2c_write(sensor, CW_WATCHDOG, &test, 1);
			break;

	case TEST2:
			printk(KERN_DEBUG "CWMCU TEST2\n");
			cwmcu_reinit();
			break;

	default:
			break;
	}
	cwmcu_powermode_switch(SWITCH_POWER_ENABLE, 0);
	return count;
}

static int set_firmware_update_data(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	printk(KERN_DEBUG "CWMCU Write Data\n");
	printk(buf);
	sensor->firmwave_update_status = 1;
	sensor->firmwave_update_status = CWMCU_Write_Mcu_Memory(buf);
	return count;
}

static int get_firmware_update_status(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk(KERN_DEBUG "CWMCU firmwave_update_status = %d\n", sensor->firmwave_update_status);
	return snprintf(buf, sizeof(buf), "%d\n", sensor->firmwave_update_status);
}

static int set_firmware_update_i2(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int intsize = sizeof(int);
	memcpy(&sensor->cw_i2c_rw, buf, intsize);
	memcpy(&sensor->cw_i2c_len, &buf[4], intsize);
	memcpy(sensor->cw_i2c_data, &buf[8], sensor->cw_i2c_len);

	return count;
}

static int get_firmware_update_i2(struct device *dev, struct device_attribute *attr, char *buf)
{
	int status = 0;
	if (sensor->cw_i2c_rw) {
		if (CWMCU_write_serial(sensor->cw_i2c_data, sensor->cw_i2c_len) < 0) {
			status = -1;
		}
		memcpy(buf, &status, sizeof(int));
		return 4;
	} else {
		if (CWMCU_read_serial(sensor->cw_i2c_data, sensor->cw_i2c_len) < 0) {
			status = -1;
			memcpy(buf, &status, sizeof(int));
			return 4;
		}
		memcpy(buf, &status, sizeof(int));
		memcpy(&buf[4], sensor->cw_i2c_data, sensor->cw_i2c_len);
		return 4+sensor->cw_i2c_len;
	}
	return  0;
}

static int mcu_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, sizeof(buf), "%d\n", sensor->mcu_mode);
}

static int mcu_model_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int mode = 0;
	sscanf(buf, "%d\n", &mode);
	CWMCU_Set_Mcu_Mode(mode);
	return count;
}

/* get calibrator data */
static int get_calibrator_data(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i = 0;
	uint8_t data[33] = {0};

	data[0] = sensor->cal_cmd;
	data[1] = sensor->cal_type;
	data[2] = sensor->cal_id;

	cwmcu_powermode_switch(SWITCH_POWER_CALIB, 1);

	switch (sensor->cal_cmd) {

	case CWMCU_CALIBRATOR_STATUS:
			printk(KERN_DEBUG "--CWMCU-- CWMCU_CALIBRATOR_STATUS\n");
			if (CWMCU_i2c_read(sensor, CW_CALIBRATOR_STATUS, &data[3], 1) >= 0) {
				printk(KERN_DEBUG "--CWMCU-- calibrator status = %d\n", data[3]);
				cwmcu_powermode_switch(SWITCH_POWER_CALIB, 0);
				return sprintf(buf, "%d\n",data[3]);
			} else {
				printk(KERN_DEBUG "--CWMCU-- i2c calibrator status = %d\n", data[3]);
				cwmcu_powermode_switch(SWITCH_POWER_CALIB, 0);
				return sprintf(buf, "fuck: %d\n",data[3]);
			}
			break;
	case CWMCU_ACCELERATION_CALIBRATOR:
			printk(KERN_DEBUG "--CWMCU-- CWMCU_ACCELERATION_CALIBRATOR read data\n");
			if (CWMCU_i2c_read(sensor, CW_CALIBRATOR_BIAS_ACC, &data[3], 30) <= 0) {
				printk(KERN_ERR "--CWMCU-- i2c calibrator read fail!!! [ACC]\n");
			}
			break;
	case CWMCU_MAGNETIC_CALIBRATOR:
			printk(KERN_DEBUG "--CWMCU-- CWMCU_MAGNETIC_CALIBRATOR read data\n");
			if (CWMCU_i2c_read(sensor, CW_CALIBRATOR_BIAS_MAG, &data[3], 30) <= 0) {
				printk(KERN_ERR "--CWMCU-- i2c calibrator read fail!!! [MAG]\n");
			}
			break;
	case CWMCU_GYRO_CALIBRATOR:
			printk(KERN_DEBUG "--CWMCU-- CWMCU_GYRO_CALIBRATOR read data\n");
			if (CWMCU_i2c_read(sensor, CW_CALIBRATOR_BIAS_GYRO, &data[3], 30) <= 0) {
				printk(KERN_ERR "--CWMCU-- i2c calibrator read fail!!! [GYRO]\n");
			}
			break;
	case CWMCU_LIGHT_CALIBRATOR:
			printk(KERN_DEBUG "--CWMCU-- CWMCU_LIGHT_CALIBRATOR read data\n");
			if (CWMCU_i2c_read(sensor, CW_CALIBRATOR_BIAS_LIGHT, &data[3], 30) <= 0) {
				printk(KERN_ERR "--CWMCU-- i2c calibrator read fail!!! [LIGHT]\n");
			}
			break;
	case CWMCU_PROXIMITY_CALIBRATOR:
			printk(KERN_DEBUG "--CWMCU-- CWMCU_PROXIMITY_CALIBRATOR read data\n");
			if (CWMCU_i2c_read(sensor, CW_CALIBRATOR_BIAS_PROXIMITY, &data[3], 30) <= 0) {
				printk(KERN_ERR "--CWMCU-- i2c calibrator read fail!!! [PROX]\n");
			}
			break;
	}

	for (i = 0; i < 33; i++) {
		printk(KERN_DEBUG "--CWMCU-- castor read data[%d] = %u\n", i, data[i]);
	}

	cwmcu_powermode_switch(SWITCH_POWER_CALIB, 0);

	return sprintf(buf, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
			data[0], data[1], data[2],
			data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12],
			data[13], data[14], data[15], data[16], data[17], data[18], data[19], data[20], data[21], data[22],
			data[23], data[24], data[25], data[26], data[27], data[28], data[29], data[30], data[31], data[32]);

}

static int set_calibrator_data(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int i = 0;
	uint8_t data[33] = {0};
	int temp[33] = {0};
	char source[512];
	char *pch;
	int buf_count=0;

	char *myBuf= source;

	strcpy(source,buf);

	printk(KERN_DEBUG "--CWMCU-- source = %s | count:%d\n", source, count);

	while ((pch = strsep(&myBuf, ", ")) != NULL) {
		buf_count++;
	}

	cwmcu_powermode_switch(SWITCH_POWER_CALIB, 1);

	printk(KERN_DEBUG "--CWMCU-- buf = %s | bufcount:%d\n", buf, buf_count);

	if (buf_count == 3) {
		sscanf(buf, "%d %d %d",&temp[0], &temp[1], &temp[2]);
		sensor->cal_cmd = (uint8_t)temp[0];
		sensor->cal_type = (uint8_t)temp[1];
		sensor->cal_id = (uint8_t)temp[2];
		printk(KERN_DEBUG "--CWMCU-- cmd:%d type:%d id:%d\n", sensor->cal_cmd, sensor->cal_type, sensor->cal_id);
		if (sensor->cal_cmd == CWMCU_CALIBRATOR_INFO) {
			printk(KERN_DEBUG "--CWMCU-- set calibrator info\n");
			CWMCU_i2c_write(sensor, CW_CALIBRATOR_TYPE, &sensor->cal_type, 1);
			CWMCU_i2c_write(sensor, CW_CALIBRATOR_SENSORLIST, &sensor->cal_id, 1);
		}
	} else if (buf_count >= 33) {
		sscanf(buf, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
			&temp[0], &temp[1], &temp[2],
			&temp[3], &temp[4], &temp[5], &temp[6], &temp[7], &temp[8], &temp[9], &temp[10], &temp[11], &temp[12],
			&temp[13], &temp[14], &temp[15], &temp[16], &temp[17], &temp[18], &temp[19], &temp[20], &temp[21], &temp[22],
			&temp[23], &temp[24], &temp[25], &temp[26], &temp[27], &temp[28], &temp[29], &temp[30], &temp[31], &temp[32]);

		for (i = 0; i < 33; i++) {
			data[i] = (uint8_t)temp[i];
		}

		sensor->cal_cmd = data[0];
		sensor->cal_type = data[1];
		sensor->cal_id = data[2];

		printk(KERN_DEBUG "--CWMCU-- set command=%d , type=%d, sensorid=%d\n", sensor->cal_cmd, sensor->cal_type, sensor->cal_id);

		switch (sensor->cal_cmd) {

		case CWMCU_ACCELERATION_CALIBRATOR:
				printk(KERN_DEBUG "--CWMCU-- CW_ACCELERATION_CALIBRATOR write data\n");
				if (CWMCU_write_i2c_block(sensor, CW_CALIBRATOR_BIAS_ACC, &data[3], 30) >= 0) {
					for (i = 0; i < 33; i++) {
						printk(KERN_DEBUG "--CWMCU-- data[%d] = %d\n", i, data[i]);
					}
				}
				break;
		case CWMCU_MAGNETIC_CALIBRATOR:
				printk(KERN_DEBUG "--CWMCU-- CWMCU_MAGNETIC_CALIBRATOR write data\n");
				if (CWMCU_write_i2c_block(sensor, CW_CALIBRATOR_BIAS_MAG, &data[3], 30) >= 0) {
					for (i = 0; i < 33; i++) {
						printk(KERN_DEBUG "--CWMCU-- data[%d] = %d\n", i, data[i]);
					}
				}
				break;
		case CWMCU_GYRO_CALIBRATOR:
				printk(KERN_DEBUG "--CWMCU-- CWMCU_GYRO_CALIBRATOR write data\n");
				if (CWMCU_write_i2c_block(sensor, CW_CALIBRATOR_BIAS_GYRO, &data[3], 30) >= 0) {
					for (i = 0; i < 33; i++) {
						printk(KERN_DEBUG "--CWMCU-- data[%d] = %d\n", i, data[i]);
					}
				}
				break;
		case CWMCU_LIGHT_CALIBRATOR:
				printk(KERN_DEBUG "--CWMCU-- CWMCU_GYRO_CALIBRATOR write data\n");
				if (CWMCU_write_i2c_block(sensor, CW_CALIBRATOR_BIAS_LIGHT, &data[3], 30) >= 0) {
					for (i = 0; i < 33; i++) {
						printk(KERN_DEBUG "--CWMCU-- data[%d] = %d\n", i, data[i]);
					}
				}
				break;
		case CWMCU_PROXIMITY_CALIBRATOR:
				printk(KERN_DEBUG "--CWMCU-- CWMCU_GYRO_CALIBRATOR write data\n");
				if (CWMCU_write_i2c_block(sensor, CW_CALIBRATOR_BIAS_PROXIMITY, &data[3], 30) >= 0) {
					for (i = 0; i < 33; i++) {
						printk(KERN_DEBUG "--CWMCU-- data[%d] = %d\n", i, data[i]);
					}
				}
				break;
		}
	} else {
		printk(KERN_DEBUG "--CWMCU-- input parameter incorrect !!! | %d\n",count);
	}

	cwmcu_powermode_switch(SWITCH_POWER_CALIB, 0);

	return count;
}

static int pcba_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	u8 data = 0;
	
	printk("--CWMCU-- %s\n", __func__);
	
	cwmcu_powermode_switch(SWITCH_POWER_PCBA, 1);
	
	if (CWMCU_i2c_read(sensor, CW_PCBA_ST, &data, 1) >= 0){
		printk("--CWMCU-- pcba get status ok,data = %d.\n",data);
	}

	cwmcu_powermode_switch(SWITCH_POWER_PCBA, 0);
	
	return snprintf(buf, 10, "%d\n", data);
}

static int pcba_set(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int err = 0;
	u8 data = 0x01; /* enable pcba check */
	u8 i = 0;
	
	printk("--CWMCU-- %s\n", __func__);
	
	for (i=0;i<10;i++) {
	
		cwmcu_powermode_switch(SWITCH_POWER_PCBA, 1);
		err = CWMCU_i2c_write(sensor, CW_PCBA_CHECK, &data, 1);
		if (err < 0) {
			printk("--CWMCU-- pcba i2c set error.\n");
		}
		cwmcu_powermode_switch(SWITCH_POWER_PCBA, 0);
		if (err) {
			msleep(10);
			err = 0;
			printk(KERN_DEBUG "--CWMCU-- pcba_set retry=%d\n", i);
		}else
			break;
	}

	return count;
}


static int powermode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk("--CWMCU-- %s\n", __func__);

	return snprintf(buf, 10, "%d\n", sensor->pwrmod);
}

static int powermode_set(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int data;

	printk("--CWMCU-- %s\n", __func__);
	sscanf(buf, "%d\n", &data);

	sensor->pwrmod = (u8)data;

	return count;
}


static int version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t data = 0;

	printk(KERN_DEBUG "--CWMCU-- %s\n", __func__);
	
       cwmcu_powermode_switch(SWITCH_POWER_ENABLE, 1);
	if (CWMCU_i2c_read(sensor, CW_FWVERSION, &data, 1) >= 0) {
		printk(KERN_DEBUG "CHECK_FIRMWAVE_VERSION : %d\n", data);
	}
	cwmcu_powermode_switch(SWITCH_POWER_ENABLE, 0);

	return snprintf(buf, 5, "%d\n", data);
}

static int cal_check_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t data = 0;

	printk("--CWMCU-- %s\n", __func__);

	cwmcu_powermode_switch(SWITCH_POWER_ENABLE, 1);
	if (CWMCU_i2c_read(sensor, CW_IFCAL, &data, 1) >= 0) {
		printk(KERN_DEBUG "--CWMCU-- cal check: %d\n", data);
	}
	cwmcu_powermode_switch(SWITCH_POWER_ENABLE, 0);

	return snprintf(buf, 5, "%d\n", data);
}

static DEVICE_ATTR(enable, 0666, active_show, active_set);
static DEVICE_ATTR(delay_ms, 0666, interval_show, interval_set);
/* static DEVICE_ATTR(poll, 0666, poll_show, NULL); */
static DEVICE_ATTR(batch, 0666, batch_show, batch_set);
static DEVICE_ATTR(flush, 0666, flush_show, flush_set);
static DEVICE_ATTR(mcu_mode, 0666, mcu_mode_show, mcu_model_set);

static DEVICE_ATTR(firmware_update_i2c, 0666, get_firmware_update_i2, set_firmware_update_i2);
static DEVICE_ATTR(firmware_update_cmd, 0666, NULL, set_firmware_update_cmd);
static DEVICE_ATTR(firmware_update_data, 0666, NULL, set_firmware_update_data);
static DEVICE_ATTR(firmware_update_status, 0666, get_firmware_update_status, NULL);

static DEVICE_ATTR(calibrator_cmd, 0666, get_calibrator_data, set_calibrator_data);
static DEVICE_ATTR(pcba_check, 0666, pcba_show, pcba_set);
static DEVICE_ATTR(powermode, 0666, powermode_show, powermode_set);
static DEVICE_ATTR(version, 0666, version_show, NULL);
static DEVICE_ATTR(cal_check, 0666, cal_check_show, NULL);

static struct attribute *sysfs_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_delay_ms.attr,
	/* &dev_attr_poll.attr, */
	&dev_attr_batch.attr,
	&dev_attr_flush.attr,

	&dev_attr_mcu_mode.attr,
	&dev_attr_firmware_update_i2c.attr,
	&dev_attr_firmware_update_cmd.attr,
	&dev_attr_firmware_update_data.attr,
	&dev_attr_firmware_update_status.attr,

	&dev_attr_calibrator_cmd.attr,
	&dev_attr_pcba_check.attr,
	&dev_attr_powermode.attr,
	&dev_attr_version.attr,
	&dev_attr_cal_check.attr,
	NULL
};

static struct attribute_group sysfs_attribute_group = {
	.attrs = sysfs_attributes
};

/*=======input device==========*/

static void /*__devinit*/ CWMCU_init_input_device(struct CWMCU_data *sensor, struct input_dev *idev)
{
	idev->name = CWMCU_I2C_NAME;
	idev->id.bustype = BUS_I2C;
	idev->dev.parent = &sensor->client->dev;
	idev->evbit[0] = BIT_MASK(EV_ABS) | BIT_MASK(EV_ABS);
	set_bit(EV_KEY, idev->evbit);

	/* send mouse event */
	set_bit(BTN_MOUSE, idev->keybit);
	set_bit(EV_REL, idev->evbit);
	set_bit(REL_X, idev->relbit);
	set_bit(REL_Y, idev->relbit);
	set_bit(EV_MSC, idev->evbit);
	set_bit(MSC_SCAN, idev->mscbit);
	set_bit(BTN_LEFT, idev->keybit);
	set_bit(BTN_RIGHT, idev->keybit);

	input_set_capability(idev, EV_KEY, 116);
	input_set_capability(idev, EV_KEY, 102);
	/*
	input_set_capability(idev, EV_KEY, 88);
	*/
	set_bit(EV_ABS, idev->evbit);
	input_set_abs_params(idev, CW_ABS_X, -DPS_MAX, DPS_MAX, 0, 0);
	input_set_abs_params(idev, CW_ABS_Y, -DPS_MAX, DPS_MAX, 0, 0);
	input_set_abs_params(idev, CW_ABS_Z, -DPS_MAX, DPS_MAX, 0, 0);
	input_set_abs_params(idev, CW_ABS_X1, -DPS_MAX, DPS_MAX, 0, 0);
	input_set_abs_params(idev, CW_ABS_Y1, -DPS_MAX, DPS_MAX, 0, 0);
	input_set_abs_params(idev, CW_ABS_Z1, -DPS_MAX, DPS_MAX, 0, 0);
	input_set_abs_params(idev, CW_ABS_TIMEDIFF, -DPS_MAX, DPS_MAX, 0, 0);
	input_set_abs_params(idev, CW_ABS_ACCURACY, -DPS_MAX, DPS_MAX, 0, 0);
	input_set_abs_params(idev, REL_X, -DPS_MAX, DPS_MAX, 0, 0);
	input_set_abs_params(idev, REL_Y, -DPS_MAX, DPS_MAX, 0, 0);
}

/*=======polling device=========*/
static void CWMCU_poll(struct input_polled_dev *dev)
{
	#ifndef CWMCU_INTERRUPT
	CWMCU_read(dev->private);
	#endif
}

static int CWMCU_open(struct CWMCU_data *sensor)
{
	int error;
	error = pm_runtime_get_sync(&sensor->client->dev);
	if (error && error != -ENOSYS)
		return error;
	return 0;
}

static void CWMCU_close(struct CWMCU_data *sensor)
{
	pm_runtime_put_sync(&sensor->client->dev);
}

static void CWMCU_poll_open(struct input_polled_dev *ipoll_dev)
{
	struct CWMCU_data *sensor = ipoll_dev->private;
	CWMCU_open(sensor);
}

static void CWMCU_poll_close(struct input_polled_dev *ipoll_dev)
{
	struct CWMCU_data *sensor = ipoll_dev->private;
	CWMCU_close(sensor);
}

static int /*__devinit*/ CWMCU_register_polled_device(struct CWMCU_data *sensor)
{
	int error = -1;
	struct input_polled_dev *ipoll_dev;

	/* poll device */
	ipoll_dev = input_allocate_polled_device();
	if (!ipoll_dev)
		return -ENOMEM;

	ipoll_dev->private = sensor;
	ipoll_dev->open = CWMCU_poll_open;
	ipoll_dev->close = CWMCU_poll_close;
	ipoll_dev->poll = CWMCU_poll;
	ipoll_dev->poll_interval = CWMCU_POLL_INTERVAL;
	ipoll_dev->poll_interval_min = CWMCU_POLL_MIN;
	ipoll_dev->poll_interval_max = CWMCU_POLL_MAX;

	CWMCU_init_input_device(sensor, ipoll_dev->input);

	error = input_register_polled_device(ipoll_dev);
	if (error) {
		input_free_polled_device(ipoll_dev);
		return error;
	}

	sensor->input_polled = ipoll_dev;
	sensor->input = ipoll_dev->input;

	return 0;
}

#ifdef	CONFIG_HAS_EARLYSUSPEND
static void CWMCU_early_suspend(struct early_suspend *h)
{
        sensor->pwrmod = 1;
	printk(KERN_DEBUG "--CWMCU-- CWMCU_early_suspend~!!!!\n");
	printk(KERN_DEBUG "--CWMCU-- sensor->pwrmod = %d\n", sensor->pwrmod);
	cwmcu_powermode_switch(SWITCH_POWER_INTERRUPT, 1);
	CWMCU_i2c_write(sensor, CW_MCUSLEEP, &sensor->pwrmod, 1);
	cwmcu_powermode_switch(SWITCH_POWER_INTERRUPT, 0);
}

static void CWMCU_late_resume(struct early_suspend *h)
{
        sensor->pwrmod = 0;
	printk(KERN_DEBUG "--CWMCU-- CWMCU_late_resume~!!!!\n");
	printk(KERN_DEBUG "--CWMCU-- sensor->pwrmod = %d\n", sensor->pwrmod);
	cwmcu_powermode_switch(SWITCH_POWER_INTERRUPT, 1);
	CWMCU_i2c_write(sensor, CW_MCUSLEEP, &sensor->pwrmod, 1);
	cwmcu_reinit();
	cwmcu_powermode_switch(SWITCH_POWER_INTERRUPT, 0);
}
#endif

#ifdef CWMCU_INTERRUPT
//static irqreturn_t CWMCU_interrupt_thread(int irq, void *data)
static void CWMCU_interrupt_thread(void)
{
	//printk(KERN_ERR "--CWMCU--%s in\n", __func__); 
	if (sensor->mcu_mode == CW_BOOT) {
		printk(KERN_DEBUG "--CWMCU--%s sensor->mcu_mode = %d\n", __func__, sensor->mcu_mode);
		return IRQ_HANDLED;
	}
	schedule_work(&sensor->work);

	return IRQ_HANDLED;
}

static void cwmcu_work_report(struct work_struct *work)
{
	uint8_t temp[6] = {0};
	u8 test = 0x01;
	/*
	struct CWMCU_data *sensor =
	    container_of(work, struct CWMCU_data, work);
	*/
	if (sensor->mcu_mode == CW_BOOT) {
		printk(KERN_DEBUG "--CWMCU--%s sensor->mcu_mode = %d\n", __func__, sensor->mcu_mode);
		return;
	}

	cwmcu_powermode_switch(SWITCH_POWER_INTERRUPT, 1);

	/* check mcu interrupt status */
	if (CWMCU_i2c_read(sensor, CW_INTERRUPT_STATUS, temp, 6) >= 0) {
		sensor->interrupt_status = (u32)temp[1] << 8 | (u32)temp[0];
		sensor->update_list = (u32)temp[5] << 24 | (u32)temp[4] << 16 | (u32)temp[3] << 8 | (u32)temp[2];
		printk(KERN_DEBUG "--CWMCU-- sensor->update_list~ = %d\n", sensor->update_list);
	} else {
		printk(KERN_DEBUG "--CWMCU-- check interrupt_status failed~!!\n");
	}

	if (sensor->interrupt_status & (1<<INTERRUPT_INIT)) {
		CWMCU_i2c_write(sensor, CW_WATCHDOG, &test, 1);
		cwmcu_reinit();
	}

	/* check interrupt until status is clean */
	if (sensor->interrupt_status & (1<<INTERRUPT_GESTURE)
		|| sensor->interrupt_status & (1<<INTERRUPT_BATCHTIMEOUT)
		|| sensor->interrupt_status & (1<<INTERRUPT_BATCHFULL)) {
		/* send home key to wake up system */
		/*
		input_report_key(sensor->input, 102, 1);
		input_sync(sensor->input);
		input_report_key(sensor->input, 102, 0);
		input_sync(sensor->input);
		*/
		/*
		input_report_key(sensor->input, 88, 1);
		input_sync(sensor->input);
		input_report_key(sensor->input, 88, 0);
		input_sync(sensor->input);
		*/
	}
	/* read sensor data of batch mode*/
	if (sensor->interrupt_status & (1<<INTERRUPT_BATCHTIMEOUT) || sensor->interrupt_status & (1<<INTERRUPT_BATCHFULL)) {
		cwmcu_batch_read(sensor);
	}

	/* read gesture event */
	if (sensor->interrupt_status & (1<<INTERRUPT_GESTURE)) {
		cwmcu_gesture_read(sensor);
	}
	/* error log */
	if (sensor->interrupt_status & (1<<INTERRUPT_ERROR)) {
		cwmcu_debuglog();
	}
	/* read sensor data of normal mode*/
	if (sensor->interrupt_status & (1<<INTERRUPT_DATAREADY)) {
		CWMCU_read(sensor);
	}

	cwmcu_powermode_switch(SWITCH_POWER_INTERRUPT, 0);
}
#endif

static int /*__devinit*/ CWMCU_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int error;
	int i = 0;

	printk(KERN_ERR "--CWMCU-- %s\n", __func__);
	/* mcu reset */
	mt_set_gpio_dir(GPIO_AST_CS_PIN, 1);
        mt_set_gpio_out(GPIO_AST_CS_PIN, 1);
	mt_set_gpio_dir(GPIO_AST_RST_PIN, 1);
	msleep(10);
	mt_set_gpio_out(GPIO_AST_RST_PIN, 0);
	mt_set_gpio_out(GPIO_AST_CS_PIN, 1);
	msleep(10);
	mt_set_gpio_out(GPIO_AST_CS_PIN, 0);
	msleep(10);
	mt_set_gpio_out(GPIO_AST_CS_PIN, 1);
	msleep(200);

	//lenovo start for wakeup
	mt_set_gpio_dir(GPIO_CW_MCU_WAKE_UP,1);
	mt_set_gpio_out(GPIO_CW_MCU_WAKE_UP,0);

	//for compass reset
	//GPIO_COMPASS_RST_PIN  GPIO_COMPASS_RST_PIN_M_GPIO
       //mt_set_gpio_out(GPIO_COMPASS_RST_PIN, 0);
	//msleep(100);
	//mt_set_gpio_out(GPIO_COMPASS_RST_PIN, 1);
	//gpio_direction_input(GPIO_CW_MCU_RESET);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "--CWMCU-- i2c_check_functionality error\n");
		return -EIO;
	}

	sensor = kzalloc(sizeof(struct CWMCU_data), GFP_KERNEL);
	if (!sensor) {
		printk(KERN_DEBUG "--CWMCU-- kzalloc error\n");
		return -ENOMEM;
	}

	CWI2CDMABuf_va = (u8 *)dma_alloc_coherent(NULL, 4096, &CWI2CDMABuf_pa, GFP_KERNEL);
    	if(!CWI2CDMABuf_va)
	{
    		printk("-CWMCU-- [LENOVO sensorHUB] dma_alloc_coherent error\n");
	}

	sensor->client = client;
	i2c_set_clientdata(client, sensor);

	error = CWMCU_register_polled_device(sensor);
	if (error) {
		printk(KERN_ERR "--CWMCU-- CWMCU_register_polled_device error\n");
		goto err_free_mem;
	}

	error = sysfs_create_group(&sensor->input->dev.kobj,
					&sysfs_attribute_group);
	if (error)
		goto exit_free_input;

	for (i = 0; i < CW_SENSORS_ID_END; i++) {
		sensor->sensors_time[i] = 0;
		sensor->report_period[i] = 200000;
		sensor->time_diff[i] = 0;
	}

	sensor->mcu_mode = CW_NORMAL;

	sensor->current_timeout = 0;
	sensor->timeout_count = 0;
	sensor->pwrmod = 0;

#ifdef CWMCU_INTERRUPT
#if 0
	sensor->client->irq = gpio_to_irq(GPIO_CW_MCU_INTERRUPT);

	printk(KERN_DEBUG "--CWMCU--sensor->client->irq  =%d~!!\n", sensor->client->irq);

	if (sensor->client->irq > 0) {

		error = request_threaded_irq(sensor->client->irq, NULL,
						   CWMCU_interrupt_thread,
						   IRQF_TRIGGER_RISING,
						   "cwmcu", sensor);
		if (error < 0) {
				pr_err("request irq %d failed\n", sensor->client->irq);
				goto exit_destroy_mutex;
			}
		disable_irq(sensor->client->irq);
		INIT_WORK(&sensor->work, cwmcu_work_report);
		enable_irq(sensor->client->irq);
	}
#endif
#if 0
	mt_eint_set_sens(CUST_EINT_HALL_1_NUM, MT_LEVEL_SENSITIVE);
	mt_eint_set_hw_debounce(CUST_EINT_HALL_1_NUM, CUST_EINT_HALL_1_DEBOUNCE_CN);
	mt_eint_registration(CUST_EINT_HALL_1_NUM, CUST_EINT_HALL_1_TYPE, CWMCU_interrupt_thread, 1);
	printk(KERN_ERR "[CWMCU]set EINT finished, eint_num=%d, eint_debounce_en=%d, eint_polarity=%d\n", CUST_EINT_HALL_1_NUM, CUST_EINT_DEBOUNCE_ENABLE, CUST_EINT_HALL_1_TYPE);
	mt_eint_mask(CUST_EINT_HALL_1_NUM);  
	INIT_WORK(&sensor->work, cwmcu_work_report);
	mt_eint_unmask(CUST_EINT_HALL_1_NUM);  
#endif

#if 1
	mt_eint_set_sens(CUST_EINT_MSE_NUM, MT_EDGE_SENSITIVE);
	mt_eint_set_hw_debounce(CUST_EINT_MSE_NUM, CUST_EINT_MSE_DEBOUNCE_CN);
	mt_eint_registration(CUST_EINT_MSE_NUM, CUST_EINTF_TRIGGER_RISING, CWMCU_interrupt_thread, 1);
	//printk(KERN_ERR "[CWMCU]set EINT finished, eint_num=%d, eint_debounce_en=%d, eint_polarity=%d\n", CUST_EINT_MSE_NUM, CUST_EINT_DEBOUNCE_ENABLE, CUST_EINTF_TRIGGER_RISING);
	mt_eint_mask(CUST_EINT_MSE_NUM);  
	INIT_WORK(&sensor->work, cwmcu_work_report);
	mt_eint_unmask(CUST_EINT_MSE_NUM);  
#endif

#endif

	i2c_set_clientdata(client, sensor);
	pm_runtime_enable(&client->dev);

#if CONFIG_HAS_EARLYSUSPEND
	sensor->early_drv.level    = EARLY_SUSPEND_LEVEL_STOP_DRAWING - 2;
	sensor->early_drv.suspend  = CWMCU_early_suspend;
	sensor->early_drv.resume   = CWMCU_late_resume;  
	register_early_suspend(&sensor->early_drv);
#endif

	printk(KERN_DEBUG "--CWMCU-- CWMCU_i2c_probe success!\n");

	return 0;

exit_free_input:
	input_free_device(sensor->input);
err_free_mem:
exit_destroy_mutex:
	free_irq(sensor->client->irq, sensor);
	kfree(sensor);
	return error;
}

static int CWMCU_i2c_remove(struct i2c_client *client)
{
	struct CWMCU_data *sensor = i2c_get_clientdata(client);
	kfree(sensor);
	if(CWI2CDMABuf_va)
	{
		dma_free_coherent(NULL, 4096, CWI2CDMABuf_va, CWI2CDMABuf_pa);
		CWI2CDMABuf_va = NULL;
		CWI2CDMABuf_pa = 0;
	}
	return 0;
}

#ifndef	CONFIG_HAS_EARLYSUSPEND
static const struct dev_pm_ops CWMCU_pm_ops = {
	.suspend = CWMCU_suspend,
	.resume = CWMCU_resume
};
#endif

static const struct i2c_device_id CWMCU_id[] = {
	{ CWMCU_I2C_NAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, CWMCU_id);

static struct i2c_driver CWMCU_driver = {
	.driver = {
		.name = CWMCU_I2C_NAME,
		.owner = THIS_MODULE,
#ifndef	CONFIG_HAS_EARLYSUSPEND
		.pm = &CWMCU_pm_ops,
#endif
	},
	.probe    = CWMCU_i2c_probe,
	.remove   = /*__devexit_p*/(CWMCU_i2c_remove),
	.id_table = CWMCU_id,
};

static int __init CWMCU_i2c_init(void){
	//printk(KERN_ERR "CWMCU_i2c_init\n");
	i2c_register_board_info(3, &CwMcuSensor_i2c3_boardinfo, 1);
	return i2c_add_driver(&CWMCU_driver);
}

static void __exit CWMCU_i2c_exit(void){
	i2c_del_driver(&CWMCU_driver);
}

module_init(CWMCU_i2c_init);
module_exit(CWMCU_i2c_exit);

MODULE_DESCRIPTION("CWMCU I2C Bus Driver");
MODULE_AUTHOR("CyWee Group Ltd.");
MODULE_LICENSE("GPL");
