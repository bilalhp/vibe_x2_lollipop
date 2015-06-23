#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>

#include <cust_acc.h>
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <linux/hwmsen_helper.h>
#include <linux/xlog.h>

#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>

#include "bq27531.h"

#include "cust_charging.h"
#include <mach/charging.h>

#include <linux/vmalloc.h>
#include <asm/unaligned.h>

#include <linux/dma-mapping.h>

#include <mach/battery_common.h>
#include <mach/mt_boot.h>

/**********************************************************
  *
  *   [I2C Slave Setting] 
  *
  *********************************************************/
#define BQ27531_DEBUG_ENABLE	
#define BQ27531_SLAVE_ADDR_WRITE	0xAA
#define BQ27531_SLAVE_ADDR_READ		0xAB

#define BQ27531_SLAVE_ADDR_ROM	0x16

static unsigned  int chipid = 0;
static unsigned  int  fwver = 0;
static unsigned  int   dfver = 0;
static unsigned  int   g_fw_valid = 0;
static unsigned int g_fw_exist = 0;
static unsigned int g_first_boot = 1;
static void work_dlfw(struct work_struct *data);
static struct work_struct work_dl_fw;
extern int g_bat_init_flag;
extern int g_platform_boot_mode;

static unsigned short bq27531_cmd_addr[bq27531_REG_NUM] = 
{
	0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e,
	0x10, 0x12, 0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e, 
	0x20, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2c, 0x2e,
	0x30, 0x32, 0x34, 0x6e, 0x70, 0x72
};

static struct i2c_client *new_client = NULL;
static const struct i2c_device_id bq27531_i2c_id[] = {{"bq27531",0},{}};   
kal_bool fg_hw_init_done = KAL_FALSE; 
static int bq27531_driver_probe(struct i2c_client *client, const struct i2c_device_id *id);
//XTWOROWL-310
extern kal_int32 fgauge_read_v_by_capacity_bq27531(int bat_capacity,kal_int32 temperature);
//XTWOROWL-310
static struct i2c_driver bq27531_driver = {
    .driver = {
        .name    = "bq27531",
    },
    .probe       = bq27531_driver_probe,
    .id_table    = bq27531_i2c_id,
};

static u8 *I2CDMABuf_va = NULL;
static u32 I2CDMABuf_pa = NULL;

/**********************************************************
  *
  *   [Global Variable] 
  *
  *********************************************************/
kal_uint16 bq27531_reg[bq27531_REG_NUM] = {0};

static DEFINE_MUTEX(bq27531_i2c_access);
static DEFINE_MUTEX(bq27531_dl_fw);

/**********************************************************
  *
  *   [I2C Function For Read/Write fan5405] 
  *
  *********************************************************/
int bq27531_read_byte(kal_uint8 cmd, kal_uint8 *returnData)
{
    char     cmd_buf[1]={0x00};
    char     readData = 0;
    int      ret=0;

    mutex_lock(&bq27531_i2c_access);

    //new_client->addr = ((new_client->addr) & I2C_MASK_FLAG) | I2C_WR_FLAG;    
    new_client->ext_flag=((new_client->ext_flag ) & I2C_MASK_FLAG ) | I2C_WR_FLAG | I2C_DIRECTION_FLAG;
    new_client->timing = 100;

    cmd_buf[0] = cmd;

    ret = i2c_master_send(new_client, &cmd_buf[0], (1<<8 | 1));
    if (ret < 0) 
    {    
        //new_client->addr = new_client->addr & I2C_MASK_FLAG;
        new_client->ext_flag=0;
        mutex_unlock(&bq27531_i2c_access);
        return 0;
    }
    
    readData = cmd_buf[0];
    *returnData = readData;

    // new_client->addr = new_client->addr & I2C_MASK_FLAG;
    new_client->ext_flag=0;
    
    mutex_unlock(&bq27531_i2c_access);    

    return 1;
}

int bq27531_write_byte(kal_uint8 cmd, kal_uint8 writeData)
{
    char    write_data[2] = {0};
    int     ret=0;

   // printk("[bq27531] bq27531_write_byte,cmd=0x%x, data=0x%x\n", cmd, writeData);

    mutex_lock(&bq27531_i2c_access);
    
    write_data[0] = cmd;
    write_data[1] = writeData;
    
    new_client->ext_flag=((new_client->ext_flag ) & I2C_MASK_FLAG ) | I2C_DIRECTION_FLAG;
    new_client->timing = 100;
	
    ret = i2c_master_send(new_client, write_data, 2);
	//printk("[bq27531] bq27531_write_byte,ret=%d\n", ret);
    if (ret < 0) 
    {
       
        new_client->ext_flag=0;
        mutex_unlock(&bq27531_i2c_access);
        return 0;
    }
    
    new_client->ext_flag=0;
    mutex_unlock(&bq27531_i2c_access);

    return 1;
}

int bq27531_read_2byte(kal_uint8 cmd, kal_uint16 *returnData)
{
    char     cmd_buf[2]={0x00};
    char     readData = 0;
    int      ret=0;

    mutex_lock(&bq27531_i2c_access);
    
    //new_client->addr = ((new_client->addr) & I2C_MASK_FLAG) | I2C_WR_FLAG;    
    new_client->ext_flag=((new_client->ext_flag ) & I2C_MASK_FLAG ) | I2C_WR_FLAG | I2C_DIRECTION_FLAG;
    new_client->timing = 100;
	
    cmd_buf[0] = cmd;

    ret = i2c_master_send(new_client, &cmd_buf[0], (2<<8 | 1));
    if (ret < 0) 
    {    
        //new_client->addr = new_client->addr & I2C_MASK_FLAG;
        new_client->ext_flag=0;

        mutex_unlock(&bq27531_i2c_access);
        return 0;
    }
    
    *returnData = (kal_uint16) (((cmd_buf[1]<<8)&0xff00) | (cmd_buf[0]&0xff));
    //*returnData = readData;

    // new_client->addr = new_client->addr & I2C_MASK_FLAG;
    new_client->ext_flag=0;
    
    mutex_unlock(&bq27531_i2c_access);    
    return 1;
}

int bq27531_write_2byte(kal_uint8 cmd, kal_uint16 writeData)
{
    char    write_data[4] = {0};
    int     ret=0;
    
    mutex_lock(&bq27531_i2c_access);
    
    write_data[0] = cmd;

    write_data[1] = (char) writeData&0xff;
    write_data[2] = (char) ((writeData>>8)&0xff);

    new_client->ext_flag=((new_client->ext_flag ) & I2C_MASK_FLAG ) | I2C_DIRECTION_FLAG;
    new_client->timing = 100;
	
    ret = i2c_master_send(new_client, write_data, 4);
    if (ret < 0) 
    {
       
        new_client->ext_flag=0;
        mutex_unlock(&bq27531_i2c_access);
        return 0;
    }
    
    new_client->ext_flag=0;
    mutex_unlock(&bq27531_i2c_access);
    return 1;
}

int bq27531_read_bytes(kal_uint8 slave_addr, kal_uint8 *returnData, kal_uint32 len)
{
	kal_uint8* buf;
	int ret=0;

	mutex_lock(&bq27531_i2c_access);

	new_client->addr = (slave_addr>>1);	
	//new_client->addr = ((new_client->addr) & I2C_MASK_FLAG) | I2C_WR_FLAG;    
	new_client->ext_flag=((new_client->ext_flag ) & I2C_MASK_FLAG ) | I2C_WR_FLAG | I2C_DIRECTION_FLAG;
        new_client->timing = 100;
		
	//printk("ww_debug read 0x%x :", returnData[0]);
	ret = i2c_master_send(new_client, &returnData[0], ((len-1)<<8 | 1));
	//	printk("[bq27531] bq27531_read_byte,ret=%d, data=0x%x\n", ret, cmd_buf[0]);	
	if (ret < 0) 
	{    
		//new_client->addr = new_client->addr & I2C_MASK_FLAG;
		new_client->ext_flag=0;
		new_client->addr = (BQ27531_SLAVE_ADDR_WRITE>>1);

		printk("[bq27531] bq27531_read_byte,ret<0 err\n");
		mutex_unlock(&bq27531_i2c_access);
		//kfree(buf);

		return 0;
	}

	//for(ret=0;ret<len-1;ret++)
	//	printk(" 0x%x", returnData[ret]);
	//printk(" \n");

	//memcpy(returnData, &buf[0], len-1);

	// new_client->addr = new_client->addr & I2C_MASK_FLAG;
	new_client->ext_flag=0;
	new_client->addr = (BQ27531_SLAVE_ADDR_WRITE>>1);    
	mutex_unlock(&bq27531_i2c_access);  

	//kfree(buf);
	return ret;
}

int bq27531_write_bytes(kal_uint8 slave_addr, kal_uint8* writeData, kal_uint32 len)
{
	int     ret=0;
	//kal_uint8* buf;
	int i = 0;
	//buf = (kal_uint8*) kmalloc(sizeof(kal_uint8)*(len+1), GFP_KERNEL);
	//memcpy(&buf[1], writeData, len);

	mutex_lock(&bq27531_i2c_access);

	new_client->addr = (slave_addr>>1);

	new_client->ext_flag=((new_client->ext_flag ) & I2C_MASK_FLAG ) | I2C_DIRECTION_FLAG;
    new_client->timing = 100;
	

	//printk("ww_debug write:");
	//for(ret=0;ret<len;ret++)
	//	printk(" 0x%x", writeData[ret]);
	//printk(" \n");

	for(i = 0 ; i < len; i++)
	{
		I2CDMABuf_va[i] = writeData[i];
	}

	if(len <= 8)
	{
		//i2c_client->addr = i2c_client->addr & I2C_MASK_FLAG;
		//MSE_ERR("Sensor non-dma write timing is %x!\r\n", this_client->timing);
		i2c_master_send(new_client, writeData, len);
	}
	else
	{
		new_client->addr = new_client->addr & I2C_MASK_FLAG | I2C_DMA_FLAG;
		//MSE_ERR("Sensor dma timing is %x!\r\n", this_client->timing);
		i2c_master_send(new_client, I2CDMABuf_pa, len);
	}   

	new_client->addr = new_client->addr & I2C_MASK_FLAG;
	new_client->ext_flag=0;
	new_client->addr = (BQ27531_SLAVE_ADDR_WRITE>>1);		
	mutex_unlock(&bq27531_i2c_access);

	//kfree(buf);

	return ret;
}

int bq27531_write_single_bytes(kal_uint8 slave_addr, kal_uint8* writeData, kal_uint32 len)
{
	int     ret=0;
	int tmp=1;
	//kal_uint8* buf;

	//buf = (kal_uint8*) kmalloc(sizeof(kal_uint8)*(len+1), GFP_KERNEL);
	//memcpy(&buf[1], writeData, len);
	unsigned char buf[2];
	int i;
	
	mutex_lock(&bq27531_i2c_access);

	new_client->addr = (slave_addr>>1);

	new_client->ext_flag=((new_client->ext_flag ) & I2C_MASK_FLAG ) | I2C_DIRECTION_FLAG;
	new_client->timing = 100;

	printk("ww_debug write:");
	for(ret=0;ret<len;ret++)
		printk(" 0x%x", writeData[ret]);
	printk(" \n");

	printk("ww_debug single write:");
	for(i=0;i<len-1;i++)
	{
		buf[0] = writeData[0] + i;
		buf[1] = writeData[i+1];

		printk("data1=0x%x, data2=0x%x  |  ", buf[0], buf[1]);
		ret = i2c_master_send(new_client, buf, 2);
		if (ret < 0) 
		{
			new_client->ext_flag=0;
			new_client->addr = (BQ27531_SLAVE_ADDR_WRITE>>1);	

			mutex_unlock(&bq27531_i2c_access);

			//kfree(buf);
			return 0;
		}

		ret -= 1;
		tmp += ret;
	}
	printk(" \n");
	
	new_client->ext_flag=0;
	new_client->addr = (BQ27531_SLAVE_ADDR_WRITE>>1);		
	mutex_unlock(&bq27531_i2c_access);

	//kfree(buf);

	return tmp;
}

int bq27531_read_ctrl(kal_uint16 cmd, kal_uint16 *returnData)
{
    char     cmd_buf[4]={0x00};
    char     readData = 0;
    int      ret=0;

    mutex_lock(&bq27531_i2c_access);
    
    //new_client->addr = ((new_client->addr) & I2C_MASK_FLAG) | I2C_WR_FLAG;    
    //new_client->ext_flag=((new_client->ext_flag ) & I2C_MASK_FLAG ) | I2C_WR_FLAG | I2C_DIRECTION_FLAG;
    new_client->ext_flag=((new_client->ext_flag ) & I2C_MASK_FLAG ) | I2C_DIRECTION_FLAG;
    new_client->timing = 100;
    udelay(100);
	
    cmd_buf[0] = (char) 0x00;

    cmd_buf[1] = (char) cmd&0xff;
    cmd_buf[2] = (char) (cmd>>8)&0xff;
    udelay(100);	
    ret = i2c_master_send(new_client, &cmd_buf[0], 3);
    if (ret < 0) 
    {    
        //new_client->addr = new_client->addr & I2C_MASK_FLAG;
        new_client->ext_flag=0;
        printk("CL bq27531_get_ctrl_dfver err1 cmd=%x\n",cmd);
        mutex_unlock(&bq27531_i2c_access);
        return 0;
    }

	new_client->ext_flag=0;	
    new_client->ext_flag=((new_client->ext_flag ) & I2C_MASK_FLAG ) | I2C_WR_FLAG | I2C_DIRECTION_FLAG;
    udelay(100);
    ret = i2c_master_send(new_client, &cmd_buf[0], ((2<<8 | 1)));
    if (ret < 0) 
    {    
        //new_client->addr = new_client->addr & I2C_MASK_FLAG;
        new_client->ext_flag=0;
        printk("CL bq27531_get_ctrl_dfver err2 cmd=%x\n",cmd);
        mutex_unlock(&bq27531_i2c_access);
        return 0;
    }
	
    *returnData = (kal_uint16) (((cmd_buf[1]<<8)&0xff00) | (cmd_buf[0]&0xff));
    //*returnData = readData;

    // new_client->addr = new_client->addr & I2C_MASK_FLAG;
    new_client->ext_flag=0;
    
    mutex_unlock(&bq27531_i2c_access);    
    return 1;
}

int bq27531_write_ctrl(kal_uint16 cmd, kal_uint16 writeData)
{
    char    write_data[6] = {0};
    int     ret=0;
    
    mutex_lock(&bq27531_i2c_access);
    
    write_data[0] = (char) 0x00;

    write_data[1] = (char) cmd&0xff;
    write_data[2] = (char) (cmd>>8)&0xff;

    write_data[3] = (char) writeData&0xff;
    write_data[4] = (char) (writeData>>8)&0xff;

	for(ret=0;ret<5;ret++)
		printk("ww_debug data[%d]=0x%x\n", ret, write_data[ret]);
	
    new_client->ext_flag=((new_client->ext_flag ) & I2C_MASK_FLAG ) | I2C_DIRECTION_FLAG;
	new_client->timing = 100;
    
    ret = i2c_master_send(new_client, write_data, 5);
    if (ret < 0) 
    {
       
        new_client->ext_flag=0;
        mutex_unlock(&bq27531_i2c_access);
        return 0;
    }
    
    new_client->ext_flag=0;
    mutex_unlock(&bq27531_i2c_access);
    return 1;
}

int bq27531_write_ctrl_cmd(kal_uint16 cmd)
{
    char    write_data[6] = {0};
    int     ret=0;
    
    mutex_lock(&bq27531_i2c_access);
    
    write_data[0] = (char) 0x00;

    write_data[1] = (char) cmd&0xff;
    write_data[2] = (char) (cmd>>8)&0xff;

	//for(ret=0;ret<5;ret++)
	//	printk("ww_debug data[%d]=0x%x\n", ret, write_data[ret]);
	
    new_client->ext_flag=((new_client->ext_flag ) & I2C_MASK_FLAG ) | I2C_DIRECTION_FLAG;
	new_client->timing = 100;
    
    ret = i2c_master_send(new_client, write_data, 3);
    if (ret < 0) 
    {
       
        new_client->ext_flag=0;
        mutex_unlock(&bq27531_i2c_access);
        return 0;
    }
    
    new_client->ext_flag=0;
    mutex_unlock(&bq27531_i2c_access);
    return 1;
}
/**********************************************************
  *
  *   [Read / Write Function] 
  *
  *********************************************************/
kal_uint32 bq27531_read_interface (kal_uint8 RegNum, kal_uint8 *val, kal_uint8 MASK, kal_uint8 SHIFT)
{
    kal_uint8 bq27531_reg = 0;
    int ret = 0;

   battery_xlog_printk(BAT_LOG_FULL,"--------------------------------------------------\n");

    ret = bq27531_read_byte(RegNum, &bq27531_reg);

	battery_xlog_printk(BAT_LOG_FULL,"[bq27531_read_interface] Reg[%x]=0x%x\n", RegNum, bq27531_reg);
	
    bq27531_reg &= (MASK << SHIFT);
    *val = (bq27531_reg >> SHIFT);
	
	battery_xlog_printk(BAT_LOG_FULL,"[bq27531_read_interface] val=0x%x\n", *val);
	
    return ret;
}

kal_uint32 bq27531_config_interface (kal_uint8 RegNum, kal_uint8 val, kal_uint8 MASK, kal_uint8 SHIFT)
{
    kal_uint8 bq27531_reg = 0;
    int ret = 0;

    battery_xlog_printk(BAT_LOG_FULL,"--------------------------------------------------\n");

    ret = bq27531_read_byte(RegNum, &bq27531_reg);
    battery_xlog_printk(BAT_LOG_FULL,"[bq27531_config_interface] Reg[%x]=0x%x\n", RegNum, bq27531_reg);
    
    bq27531_reg &= ~(MASK << SHIFT);
    bq27531_reg |= (val << SHIFT);

    ret = bq27531_write_byte(RegNum, bq27531_reg);
    battery_xlog_printk(BAT_LOG_FULL,"[bq27531_config_interface] write Reg[%x]=0x%x\n", RegNum, bq27531_reg);

    // Check
    //bq27531_read_byte(RegNum, &bq27531_reg);
    //printk("[bq27531_config_interface] Check Reg[%x]=0x%x\n", RegNum, bq27531_reg);

    return ret;
}

//write one register directly
kal_uint32 bq27531_config_interface_liao (kal_uint8 RegNum, kal_uint8 val)
{   
    int ret = 0;
    
    ret = bq27531_write_byte(RegNum, val);

    return ret;
}

/**********************************************************
  *
  *   [Internal Function] 
  *
  *********************************************************/

//command 0 : control
unsigned int bq27531_get_ctrl_devicetype(void)
{
	kal_uint32 ret=0;    
	kal_uint32 val = 0;

	ret=bq27531_read_ctrl(bq27531_CTRL_DEVTYPE, (kal_uint16*) &val);
	if(ret<=0)
		val = 0xffff;
	
	return val;
}

unsigned int bq27531_get_ctrl_chipstatus(void)
{
	kal_uint32 ret=0;    
	kal_uint32 val = 0;

	ret=bq27531_read_ctrl(bq27531_CTRL_STATUS, (kal_uint16*) &val);	
	if(ret<=0)
		val = 0xffff;
	
	return val;
}

unsigned int bq27531_get_ctrl_fwver(void)
{
    	kal_uint32 ret=0;    
	kal_uint32 val = 0;

    	ret=bq27531_read_ctrl(bq27531_CTRL_FWVER, (kal_uint16*) &val);
	if(ret<=0)
		val = 0xffff;
	
	return val;
}


unsigned int bq27531_get_ocv(void)
{
    kal_uint32 ret=0;     		
	kal_uint32 val = 0;

    ret=bq27531_read_ctrl(bq27531_CTRL_OCVCMD, (kal_uint16*) &val);
	if(ret<=0)
		val = 0xffff;
	
	return val;
}
void bq27531_ctrl_enableotg(void)
{
    kal_uint32 ret=0;    

    ret=bq27531_write_2byte(bq27531_CMD_CONTROL, bq27531_CTRL_OTGENABLE);

	return ret;
}

void bq27531_ctrl_disableotg(void)
{
    kal_uint32 ret=0;    

    ret=bq27531_write_2byte(bq27531_CMD_CONTROL, bq27531_CTRL_OTGDISABLE);

	return ret;
}

unsigned int bq27531_ctrl_ctrlenablecharge(void)
{
    kal_uint32 ret=0;    

    ret=bq27531_write_2byte(bq27531_CMD_CONTROL, bq27531_CTRL_CHGCTLENABLE);
	
	return ret;
}

unsigned int bq27531_ctrl_ctrldisablecharge(void)
{
    kal_uint32 ret=0;    

    ret=bq27531_write_2byte(bq27531_CMD_CONTROL, bq27531_CTRL_CHGCTLDISABLE);
	
	return ret;
}

unsigned int bq27531_ctrl_enablecharge(void)
{
    kal_uint32 ret=0;    

    ret=bq27531_write_2byte(bq27531_CMD_CONTROL, bq27531_CTRL_CHGENABLE);

	return ret;
}

unsigned int bq27531_ctrl_disablecharge(void)
{
    kal_uint32 ret=0;    

    ret=bq27531_write_2byte(bq27531_CMD_CONTROL, bq27531_CTRL_CHGDISABLE);

	return ret;
}

unsigned int bq27531_get_ctrl_dfver(void)
{
    	kal_uint32 ret=0;    
	kal_uint32 val = 0;

   	 ret=bq27531_read_ctrl(bq27531_CTRL_DFVER, (kal_uint16*) &val);
	 printk("CL bq27531_get_ctrl_dfver ret=%d\n",ret);
	if(ret<=0)
		val = 0xffff;
	

	return val;
}


unsigned int bq27531_get_ctrl_status(void)
{
    	kal_uint32 ret=0;    
	kal_uint32 val = 0;

    	ret=bq27531_read_ctrl(bq27531_CTRL_STATUS, (kal_uint16*) &val);
	if(ret<=0)
		val = 0xffff;
	
	return val;
}
kal_int32 bq27531_get_flag(void)
{
	unsigned short ret = 0;
	
	bq27531_read_2byte(bq27531_CMD_FLAG, &ret);
	return ret;
}

kal_int32 bq27531_get_fcc(void)
{
	unsigned short ret = 0;
	
	bq27531_read_2byte(bq27531_CMD_CHARGECAP, &ret);
	return ret;
}

short bq27531_set_it_enable(void)
{
	short ret = 0;
	
	ret = bq27531_write_ctrl_cmd(bq27531_CTRL_ITENABLE);
    printk("bq27531_set_it_enable: ret=%d  \n",ret);
	
	return ret;
}

short bq27531_set_reset(void)
{
	short ret = 0;
	
	ret = bq27531_write_ctrl_cmd(bq27531_CTRL_RESET);
    printk("bq27531_set_reset: ret=%d  \n",ret);
	
	return ret;
}
void bq27531_set_temperature(kal_int32 temp)
{
    kal_int32 fg_tmp;
	short soc;

    if(g_fw_valid == 1)
    {
	    mutex_lock(&bq27531_dl_fw);

		if(g_first_boot == 1)
		{
			g_first_boot = 0;
			
            fg_tmp = bq27531_get_temperature();
			bq27531_read_2byte(bq27531_CMD_STATECHARGE, (unsigned short*)&soc);
			bq27531_write_2byte(bq27531_CMD_TEMPERATURE, (temp*10 + 2731));
		
			printk("bq27531: fg_tmp=%d,tmp=%d,soc=%d ,vol=%d, charger=%d \n",fg_tmp,temp,soc,BMT_status.bat_vol,BMT_status.charger_exist);
	
			if(((BMT_status.bat_vol > 3700)&&(soc ==0) && (BMT_status.charger_exist == KAL_FALSE) && (g_platform_boot_mode != KERNEL_POWER_OFF_CHARGING_BOOT) && (g_platform_boot_mode  != LOW_POWER_OFF_CHARGING_BOOT))
				||((temp>10)&&(BMT_status.bat_vol > 4300)&&(soc ==0) &&(g_platform_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT|| g_platform_boot_mode == LOW_POWER_OFF_CHARGING_BOOT)))		
			{
			       printk("bq27531_set_temperature: bq27531_set_reset \n");
                             bq27531_set_reset();
				bq27531_write_2byte(bq27531_CMD_TEMPERATURE, (temp*10 + 2731));
				msleep(3000);
			}
	
		}
		else
		{
	        bq27531_write_2byte(bq27531_CMD_TEMPERATURE, (temp*10 + 2731));
		}
		
	    mutex_unlock(&bq27531_dl_fw);
    }
	else
	{
        g_first_boot = 0;
	}
	
}
kal_int32 bq27531_get_temperature(void)
{
	unsigned short ret = 0;
  
	bq27531_read_2byte(bq27531_CMD_TEMPERATURE, &ret);
	return (ret-2731)/10;
	
}
unsigned short bq27531_get_voltage(void)
{
	unsigned short ret = 0;
	
	bq27531_read_2byte(bq27531_CMD_VOLTAGE, &ret);

	return ret;
}
short bq27531_get_averagecurrent(void)
{
	short ret = 0;
	
	bq27531_read_2byte(bq27531_CMD_AVGCUR, (unsigned short*)&ret);

 	printk("bq27531_get_averagecurrent avg_cur = %d\n", (int) ret);
	
	return ret;
}

short bq27531_get_instantaneouscurrent(void)
{
	short ret = 0;

	mutex_lock(&bq27531_dl_fw);
	
	bq27531_read_2byte(bq27531_CMD_CURREADING, (unsigned short*)&ret);

 	printk("bq27531_get_instantaneouscurrent ins_cur = %d\n", (int) ret);
	mutex_unlock(&bq27531_dl_fw);
	
	return ret;
}

//XTWOROWL-310
int bq27531_if_charging_full_status(void)
{
	short soc = 0;

	bq27531_read_2byte(bq27531_CMD_STATECHARGE, (unsigned short*)&soc);
	printk(KERN_ERR " charger_exist:%d,soc=%d,BMT_status.SOC:%d",BMT_status.charger_exist,soc, BMT_status.SOC);
	if(BMT_status.charger_exist == KAL_TRUE && soc == 100)
		return 1;
	else
		return 0;

}
//XTWOROWL-310
extern u64 g_battery_wakeup_pre_ms ;
short bq27531_get_percengage_of_fullchargercapacity(void)
{
	short soc = 0;
	int bat_val;
//XTWOROWL-310
	int temperature = bq27531_get_temperature();
	int bat_val_from_chart = 3700;
	static int bat_vol_counter = 0;
//XTWOROWL-310
	mutex_lock(&bq27531_dl_fw);

	bat_val = bq27531_get_voltage();
	bq27531_read_2byte(bq27531_CMD_STATECHARGE, (unsigned short*)&soc);
	
	if(((soc == 0)&&(1 == g_fw_valid)&&(bat_val>3700)&& (BMT_status.bat_vol > 3700)&&(BMT_status.charger_exist == KAL_FALSE)&& (g_bat_init_flag == KAL_TRUE))
		||((BMT_status.temperature > 18)&&(1 == g_fw_valid)&&(soc == 100)&&(bat_val<3900)&& (BMT_status.bat_vol < 3900)))	
	{
		printk("bq27531 soc recal before =%d  \n",soc);
        bq27531_set_reset();
	    bq27531_write_2byte(bq27531_CMD_TEMPERATURE, (BMT_status.temperature*10 + 2731));
		msleep(3000);
		bq27531_write_2byte(bq27531_CMD_TEMPERATURE, (BMT_status.temperature*10 + 2731));
		bq27531_read_2byte(bq27531_CMD_STATECHARGE, (unsigned short*)&soc);
		printk("bq27531 soc recal after =%d  \n",soc);
	}
//XTWOROWL-310
	bat_val_from_chart = fgauge_read_v_by_capacity_bq27531((100-soc),temperature);
	if((BMT_status.charger_exist == KAL_TRUE)&&(soc == 100))
	{
		g_battery_wakeup_pre_ms = ktime_to_ms(ktime_get());
	}

	printk(KERN_ERR "gauge reset print temperature:%d, bat_val:%d,bat_vol_from_chart:%d,pre_ms:%lld",temperature,bat_val,bat_val_from_chart,g_battery_wakeup_pre_ms );
	if(abs(bat_val-bat_val_from_chart) > 600 ) //0.6v
	{
		if(bat_vol_counter++ > 360)  //1H 
		{
			bq27531_set_reset();
			printk(KERN_ERR "voltage 0.6V for 1H trigger gauge reset");
			bat_vol_counter = 0;
		}
	}
	else
		bat_vol_counter = 0;
//XTWOROWL-310
	#ifdef BQ27531_DEBUG_ENABLE
    printk("bq27531_debug:     %x,    %x,    %d,    %d,    %d,     %d,    %d,    %d,    %x \n",
    bq27531_get_ctrl_status(),bq27531_get_flag(),bq27531_get_temperature(),bat_val,
    bq27531_get_averagecurrent(),bq27531_get_fcc(),bq27531_get_remaincap(),soc,dfver);
	#endif
    mutex_unlock(&bq27531_dl_fw);
	return soc;
}


short bq27531_get_remaincap(void)
{
	short ret = 0;
	
	bq27531_read_2byte(bq27531_CMD_REMAINCAP, (unsigned short*)&ret);

 	printk("bq27531_get_remaincap remacap = %d\n", (int) ret);
	
	return ret;
}

void bq27531_set_charge_voltage(unsigned short vol)
{
	unsigned short ret = 0;
	
	bq27531_write_2byte(bq27531_CMD_PROGCARGINGVOL, vol);

	return ret;
}

short bq27531_get_charge_voltage(void)
{
	short ret = 0;
	
	bq27531_read_2byte(bq27531_CMD_PROGCARGINGVOL, (unsigned short*)&ret);
 	printk("[bq27531] ww_debug chg vol = %d\n", (short) ret);

	return ret;
}
//------------rom mode for firmware-----------------
unsigned int bq27531_enter_rommode(void)
{
    kal_uint32 ret=0;    
	kal_uint16 cmd = 0x0f00;

    ret=bq27531_write_2byte(bq27531_CMD_CONTROL, cmd);
	
	return ret;
}
unsigned int bq27531_exit_rommode(void)
{
    	kal_uint32 ret=0;    
	kal_uint8 cmd[2] = {0x00, 0x0f};

    	ret=bq27531_write_bytes(BQ27531_SLAVE_ADDR_ROM, cmd, 2);
		
	cmd[0] = 0x64;
	cmd[1] = 0x0f;	
   	 ret=bq27531_write_bytes(BQ27531_SLAVE_ADDR_ROM, cmd, 2);
	
	cmd[0] = 0x65;
	cmd[1] = 0x00;	
    	ret=bq27531_write_bytes(BQ27531_SLAVE_ADDR_ROM, cmd, 2);
	
	return ret;
}
/**********************************************************
  *
  *   [Internal Function] 
  *
  *********************************************************/
  
void bq27531_dump_2register(void)
{
    int i=0;
unsigned char templ;
unsigned char temph;

    printk("[bq27531] ");
    for (i=2;i<bq27531_REG_NUM;i++)
    {
        bq27531_read_byte(i, &templ);
        bq27531_read_byte(i+1, &temph);
		bq27531_reg[i] = ((temph<<8)&0xff00)|templ;
        printk("bq27531_dump_2register: [0x%x]=0x%x, l=0x%x, h=0x%x \n", i, bq27531_reg[i], templ, temph);        
    }
    printk("\n");
}

void bq27531_dump_register(void)
{
    int i=0;
	unsigned int id = 0;
	
    printk("[bq27531] ");
#if 0
    for (i=1;i<bq27531_REG_NUM;i++)
    {
        bq27531_read_2byte(bq27531_cmd_addr[i], &bq27531_reg[i]);
        printk("[0x%x]=0x%x ", bq27531_cmd_addr[i], bq27531_reg[i]);        
    }
    printk("\n");
#endif

#if 0 //test id
	msleep(500);	
	id = bq27531_get_ctrl_devicetype();
 	printk("ww_debug id = 0x%x\n", id);

	id = bq27531_get_ctrl_fwver();
 	printk("ww_debug fwver = 0x%x\n", id);	
#endif

}



/**********************************************************
  *
  *   [platform_driver API] 
  *
  *********************************************************/
static  unsigned int get_fw_status()
{
        unsigned int tmp = 0;
		
              chipid = bq27531_get_ctrl_devicetype();
	       fwver = bq27531_get_ctrl_fwver();
              dfver = bq27531_get_ctrl_dfver();

               if ((dfver == BQ27531_DFVER) ||(dfver == 0x0AA0C)||(dfver == 0x0AA0B)  ||(dfver == 0x0AA0E) ||(dfver == 0x0AA0A) ||(dfver == 0x0AA05) ||(dfver == 0x0AA02))
             	{
                  g_fw_valid = 1;
		}
	       else
	       	{
                  g_fw_valid = 0;
		}
		   
		if(BQ27531_CHIPID != chipid)
		{
            tmp |= ERR_CHIPID; 
		}
		
		if(BQ27531_FWVER != fwver)
		{
            tmp |= ERR_FWVER; 
		}

		if(BQ27531_DFVER != dfver)
		{
            tmp |= ERR_DFVER; 
		}
		printk("get_fw_status: chipid =0x%x, fwver = 0x%x, dfver = 0x%x,tmp =0x%x \n", chipid, fwver, dfver,tmp);
		
        if (0 == tmp)
        {
            return 1;
		}
		else
		{
            return 0;
		}
}
static void bq27531_upload_fw(void)
{
    int i=0;
    int ret;

	mutex_lock(&bq27531_dl_fw);
    do
	{
	    printk(" store_fw start \n");
        ret = bq27531_fw_upgrade();
		i++;
		printk(" store_fw download times is %d,ret=%d \n",i,ret);
	}
	while((ret != 0) && (i < 3));
	if (ret != 0)
	{
		g_fw_exist = 0;
	}
	else
	{
        g_fw_exist = get_fw_status();
		printk(" store_fw g_fw_exist = %d \n",g_fw_exist);
	}
	
	mutex_unlock(&bq27531_dl_fw);
}

static void work_dlfw(struct work_struct *data)
{
	bq27531_upload_fw();
}

static ssize_t show_fw(struct device *dev,struct device_attribute *attr, char *buf)
{		
		mutex_lock(&bq27531_dl_fw);
		printk("show_fw: g_fw_exist =0x%x\n",g_fw_exist );
		mutex_unlock(&bq27531_dl_fw);
		
        return sprintf(buf, "%u\n", g_fw_exist);
}

static ssize_t store_fw(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    char *pvalue = NULL;
	unsigned int download_cmd = 0;
	
    printk( "store_fw in\n");
    if(buf != NULL && size != 0)
    {
        download_cmd = simple_strtoul(buf,&pvalue,16);  
		printk("store_fw buf is %s and size is %d,cmd is %d \n",buf,size,download_cmd);

		if(1 == g_fw_exist)
		{
			printk("store_fw, fw alreadly exist \n");
                        return size;
		}

		if(0 == download_cmd)
		{
            schedule_work(&work_dl_fw);
	    }
		else if(1 == download_cmd )
	    {
            bq27531_upload_fw();
	    }	
    }        
    return size;
}


static DEVICE_ATTR(fw, 0664, show_fw, store_fw); //664//willcai modify 2014-7-16
static ssize_t show_bq27531_ata(struct device *dev,struct device_attribute *attr, char *buf)
{		
	kal_uint8 bq27531_ata_test;
	int ret = 0;
	
    ret = bq27531_read_byte(2, &bq27531_ata_test);
	printk("show_bq27531_ata:ret=%d",ret);
   	if(ret<0)
        return sprintf(buf, "%u\n", 0);
	else
	    return sprintf(buf, "%u\n", 1);	    
}

static ssize_t store_bq27531_ata(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    return size;
}

static DEVICE_ATTR(bq27531_ata, 0664, show_bq27531_ata, store_bq27531_ata); //664

static int bq27531_driver_probe(struct i2c_client *client, const struct i2c_device_id *id) 
{             
    int err=0; 
    battery_xlog_printk(BAT_LOG_CRTI,"[bq27531_driver_probe] \n");
	
    INIT_WORK(&work_dl_fw, work_dlfw);

    if (!(new_client = kmalloc(sizeof(struct i2c_client), GFP_KERNEL))) {
        err = -ENOMEM;
        goto exit;
    }    
    memset(new_client, 0, sizeof(struct i2c_client));

    new_client = client;    

	//new_client->addr = (BQ27531_SLAVE_ADDR_WRITE>>1);

	I2CDMABuf_va = (u8 *)dma_alloc_coherent(NULL, 4096, &I2CDMABuf_pa, GFP_KERNEL);
    	if(!I2CDMABuf_va)
	{
    		printk("[BQ27531] dma_alloc_coherent error\n");
	}
		
    //---------------------
    //bq27531_dump_register();


    g_fw_exist = get_fw_status();	

	
	
    fg_hw_init_done = KAL_TRUE;
	
    return 0;                                                                                       

exit:
    return err;

}
static int bq27531_user_space_probe(struct platform_device *dev)    
{    
    int ret_device_file = 0;

    battery_xlog_printk(BAT_LOG_CRTI,"******** bq27531_user_space_probe!! ********\n" );
    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_fw);
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_bq27531_ata);
    return 0;
}

struct platform_device bq27531_user_space_device = {
    .name   = "bq27531-user",
    .id     = -1,
};

static struct platform_driver bq27531_user_space_driver = {
    .probe      = bq27531_user_space_probe,
    .driver     = {
        .name = "bq27531-user",
    },
};


static struct i2c_board_info __initdata i2c_bq27531 = { I2C_BOARD_INFO("bq27531", (BQ27531_SLAVE_ADDR_WRITE>>1))};
//static struct i2c_board_info __initdata i2c_bq27531 = { I2C_BOARD_INFO("bq27531", (0x56))};

static int __init bq27531_init(void)
{    
    int ret=0;
    
    battery_xlog_printk(BAT_LOG_CRTI,"[bq27531_init] init start\n");
    
    i2c_register_board_info(BQ27531_BUSNUM, &i2c_bq27531, 1);

    if(i2c_add_driver(&bq27531_driver)!=0)
    {
        battery_xlog_printk(BAT_LOG_CRTI,"[bq27531_init] failed to register bq27531 i2c driver.\n");
    }
    else
    {
        battery_xlog_printk(BAT_LOG_CRTI,"[bq27531_init] Success to register bq27531 i2c driver.\n");
    }

    // fan5405 user space access interface
    ret = platform_device_register(&bq27531_user_space_device);
    if (ret) {
        battery_xlog_printk(BAT_LOG_CRTI,"****[bq27531_init] Unable to device register(%d)\n", ret);
        return ret;
    }    
    ret = platform_driver_register(&bq27531_user_space_driver);
    if (ret) {
        battery_xlog_printk(BAT_LOG_CRTI,"****[bq27531_init] Unable to register driver (%d)\n", ret);
        return ret;
    }

    return 0;        
}

static void __exit bq27531_exit(void)
{
    i2c_del_driver(&bq27531_driver);
}

module_init(bq27531_init);
module_exit(bq27531_exit);
   
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C bq27531 Driver");
MODULE_AUTHOR("James Lo<james.lo@mediatek.com>");
