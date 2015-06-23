#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/delay.h>  

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

#include <mach/irqs.h>
#include <mach/mt_cirq.h>
#include <mach/mt_spm_idle.h>
#include <mach/mt_cpuidle.h>
#include <mach/mt_gpt.h>
#include <mach/mt_cpufreq.h>
#include <mach/mt_clkmgr.h>
//#include <mach/mt_secure_api.h>
#include <mach/mt_boot.h>

#include "mt_spm_internal.h"


/**************************************
 * only for internal debug
 **************************************/
//FIXME: for FPGA early porting
#define  CONFIG_MTK_LDVT

#ifdef CONFIG_MTK_LDVT
#define SPM_BYPASS_SYSPWREQ     1
#else
#define SPM_BYPASS_SYSPWREQ     0
#endif

#define SODI_DVT_APxGPT 			1 /* 0:disable, 1: enable : use in android load: mt_idle.c and mt_spm_sodi.c */
#define SODI_DVT_BLOCK_BF_WFI 		1 /* 1: using pcm_reserve to do SPM step by step test  */
#define SODI_DVT_SPM_DBG_MODE 		1 /* 1: using debug mode fw, 0: normal fw */
#define SPM_SODI_DEBUG 				0 /* 1:dump SPM registers, 0:do nothing */
#define SODI_DVT_SPM_MEM_RW_TEST 	0

#if SODI_DVT_SPM_MEM_RW_TEST
#define SODI_DVT_MAGIC_NUM 			0xa5a5a5a5 
static u32 magicArray[16]=
{	
	SODI_DVT_MAGIC_NUM,SODI_DVT_MAGIC_NUM,SODI_DVT_MAGIC_NUM,SODI_DVT_MAGIC_NUM,
	SODI_DVT_MAGIC_NUM,SODI_DVT_MAGIC_NUM,SODI_DVT_MAGIC_NUM,SODI_DVT_MAGIC_NUM,
	SODI_DVT_MAGIC_NUM,SODI_DVT_MAGIC_NUM,SODI_DVT_MAGIC_NUM,SODI_DVT_MAGIC_NUM,
	SODI_DVT_MAGIC_NUM,SODI_DVT_MAGIC_NUM,SODI_DVT_MAGIC_NUM,SODI_DVT_MAGIC_NUM,	
};
#endif


#if (SODI_DVT_APxGPT) //APxGPT test only

#if (SODI_DVT_BLOCK_BF_WFI)
#define WAKE_SRC_FOR_SODI WAKE_SRC_EINT
#else
#define WAKE_SRC_FOR_SODI WAKE_SRC_GPT
#endif

#else //#if (SODI_DVT_APxGPT)

#define WAKE_SRC_FOR_SODI \
    (WAKE_SRC_KP | WAKE_SRC_GPT | WAKE_SRC_EINT | WAKE_SRC_CONN_WDT | WAKE_SRC_CCIF0_MD | WAKE_SRC_CCIF1_MD | \
     WAKE_SRC_CONN2AP | WAKE_SRC_USB_CD | WAKE_SRC_USB_PDN | WAKE_SRC_SEJ | WAKE_SRC_AFE | WAKE_SRC_CIRQ | \
     WAKE_SRC_MD1_VRF18_WAKE | WAKE_SRC_SYSPWREQ | WAKE_SRC_MD_WDT | WAKE_SRC_MD2_WDT | WAKE_SRC_CLDMA_MD)

#endif //#if (SODI_DVT_APxGPT)


#define I2C_CHANNEL 2


#ifdef CONFIG_OF
void __iomem *spm_mcucfg;
#define K2_MCUCFG_BASE          spm_mcucfg
#else
#define K2_MCUCFG_BASE          (0xF0200000)      //0x1020_0000
#endif

#define MP0_AXI_CONFIG          (K2_MCUCFG_BASE + 0x2C) 
#define MP1_AXI_CONFIG          (K2_MCUCFG_BASE + 0x22C)
#define ACINACTM                (1<<4)


#if !SODI_DVT_SPM_DBG_MODE
//formal FW
static const u32 sodi_binary[] = {
	0x81431801, 0xd82003c5, 0x17c07c1f, 0x81411801, 0xd8000245, 0x17c07c1f,
	0x18c0001f, 0x10006240, 0xe0e00016, 0xe0e0001e, 0xe0e0000e, 0xe0e0000f,
	0x18c0001f, 0x102135cc, 0x1910001f, 0x102135cc, 0x813f8404, 0xe0c00004,
	0x803e0400, 0x1b80001f, 0x20000222, 0x80380400, 0x1b80001f, 0x20000280,
	0x803b0400, 0x1b80001f, 0x2000001a, 0x803d0400, 0x1b80001f, 0x20000208,
	0x80340400, 0x80310400, 0x81431801, 0xd82008e5, 0x17c07c1f, 0x1b80001f,
	0x2000000a, 0x18c0001f, 0x10006240, 0xe0e0000d, 0x81411801, 0xd80008a5,
	0x17c07c1f, 0x1b80001f, 0x20000020, 0x18c0001f, 0x102130f0, 0x1910001f,
	0x102130f0, 0xa9000004, 0x10000000, 0xe0c00004, 0x1b80001f, 0x2000000a,
	0x89000004, 0xefffffff, 0xe0c00004, 0x18c0001f, 0x102140f4, 0x1910001f,
	0x102140f4, 0xa9000004, 0x02000000, 0xe0c00004, 0x1b80001f, 0x2000000a,
	0x89000004, 0xfdffffff, 0xe0c00004, 0x1b80001f, 0x20000100, 0x81fa0407,
	0x81f08407, 0xe8208000, 0x10006354, 0x00defba1, 0xa1d80407, 0xa1de8407,
	0xa1df0407, 0xc2801ba0, 0x1291041f, 0x1b00001f, 0xaf7cefff, 0xf0000000,
	0x17c07c1f, 0x1a50001f, 0x10006608, 0x80c9a401, 0x810ba401, 0x10920c1f,
	0xa0979002, 0x80ca2401, 0xa0938c02, 0xa0958402, 0x8080080d, 0xd8200d62,
	0x17c07c1f, 0x1b00001f, 0x2f7ce7ff, 0x1b80001f, 0x20000004, 0xd800128c,
	0x17c07c1f, 0x1b00001f, 0xaf7cefff, 0xd800128c, 0x17c07c1f, 0x81f80407,
	0x81fe8407, 0x81ff0407, 0x1880001f, 0x10006320, 0xc0c015e0, 0xe080000f,
	0xd8000c23, 0x17c07c1f, 0xe080001f, 0xa1da0407, 0xa0110400, 0xa0140400,
	0x81431801, 0xd8201205, 0x17c07c1f, 0xa0180400, 0xa01b0400, 0xa01d0400,
	0x1b80001f, 0x20000068, 0xa01e0400, 0x1b80001f, 0x20000104, 0x81411801,
	0xd8001205, 0x17c07c1f, 0x18c0001f, 0x10006240, 0xc0c01540, 0x17c07c1f,
	0x18c0001f, 0x102135cc, 0x1910001f, 0x102135cc, 0xa11f8404, 0xe0c00004,
	0xc2801ba0, 0x1291841f, 0x1b00001f, 0x6f7ce7ff, 0xf0000000, 0x17c07c1f,
	0x80378400, 0xc2801ba0, 0x1290041f, 0x1b00001f, 0x2f7cf7ff, 0xf0000000,
	0x17c07c1f, 0x1b00001f, 0x2f7ce7ff, 0x1b80001f, 0x20000004, 0xd800150c,
	0x17c07c1f, 0xc2801ba0, 0x129f041f, 0xa0178400, 0x1b00001f, 0xaf7cefff,
	0xf0000000, 0x17c07c1f, 0xe0e0000f, 0xe0e0001e, 0xe0e00012, 0xf0000000,
	0x17c07c1f, 0x1112841f, 0xa1d08407, 0xd82016a4, 0x80eab401, 0xd8001623,
	0x01200404, 0x1a00001f, 0x10006814, 0xe2000003, 0xf0000000, 0x17c07c1f,
	0xd800180a, 0x17c07c1f, 0xe2e00036, 0xe2e0003e, 0x1380201f, 0xe2e0003c,
	0xd820194a, 0x17c07c1f, 0x1b80001f, 0x20000018, 0xe2e0007c, 0x1b80001f,
	0x20000003, 0xe2e0005c, 0xe2e0004c, 0xe2e0004d, 0xf0000000, 0x17c07c1f,
	0xa1d10407, 0x1b80001f, 0x20000020, 0xf0000000, 0x17c07c1f, 0xd8001aca,
	0x17c07c1f, 0xe2e0004f, 0xe2e0006f, 0xe2e0002f, 0xd8201b6a, 0x17c07c1f,
	0xe2e0002e, 0xe2e0003e, 0xe2e00032, 0xf0000000, 0x17c07c1f, 0x18c0001f,
	0x10006b18, 0x1910001f, 0x10006b18, 0xa1002804, 0xe0c00004, 0xf0000000,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x1840001f, 0x00000001, 0xa1d48407, 0x1990001f,
	0x10006b08, 0xe8208000, 0x10006b18, 0x00000000, 0x1b00001f, 0x2f7ce7ff,
	0x1b80001f, 0xd00f0000, 0x81469801, 0xd8204265, 0x17c07c1f, 0x8880000c,
	0x2f7ce7ff, 0xd8005402, 0x17c07c1f, 0xe8208000, 0x10006354, 0x00defba1,
	0xc0c01980, 0x81401801, 0x18c0001f, 0x10000338, 0x1910001f, 0x10000338,
	0x81308404, 0xe0c00004, 0xd8004845, 0x17c07c1f, 0x81f60407, 0x18c0001f,
	0x10006200, 0xc0c01a20, 0x12807c1f, 0xe8208000, 0x1000625c, 0x00000001,
	0x1b80001f, 0x20000080, 0xc0c01a20, 0x1280041f, 0x18c0001f, 0x10006208,
	0xc0c01a20, 0x12807c1f, 0xe8208000, 0x10006248, 0x00000000, 0x1b80001f,
	0x20000080, 0xc0c01a20, 0x1280041f, 0x18c0001f, 0x10006290, 0xc0c01a20,
	0x12807c1f, 0xc0c01a20, 0x1280041f, 0x18c0001f, 0x100062dc, 0xe0c00001,
	0xc2801ba0, 0x1292041f, 0x81469801, 0xd8004925, 0x17c07c1f, 0x8880000c,
	0x2f7ce7ff, 0xd8004ea2, 0x17c07c1f, 0x18c0001f, 0x10006294, 0xe0f07fff,
	0xe0e00fff, 0xe0e000ff, 0xa1d98407, 0xa0108400, 0xa0148400, 0xa01b8400,
	0xa0188400, 0xe8208000, 0x10006310, 0x0b1600f8, 0x1b00001f, 0x2f7cf7ff,
	0x1b80001f, 0x90100000, 0x80c10001, 0xc8c00003, 0x17c07c1f, 0x80c78001,
	0xc8c012c3, 0x17c07c1f, 0x18c0001f, 0x10006294, 0xe0e001fe, 0xe0e003fc,
	0xe0e007f8, 0xe0e00ff0, 0x1b80001f, 0x20000020, 0xe0f07ff0, 0xe0f07f00,
	0x80388400, 0x1b80001f, 0x20000300, 0x803b8400, 0x1b80001f, 0x20000300,
	0x80348400, 0x1b80001f, 0x20000104, 0x10007c1f, 0x81f38407, 0x81401801,
	0xd8005345, 0x17c07c1f, 0x18c0001f, 0x100062dc, 0xe0c0001f, 0x18c0001f,
	0x10006290, 0x1212841f, 0xc0c01740, 0x12807c1f, 0xc0c01740, 0x1280041f,
	0x18c0001f, 0x10006208, 0x1212841f, 0xc0c01740, 0x12807c1f, 0xe8208000,
	0x10006248, 0x00000001, 0x1b80001f, 0x20000080, 0xc0c01740, 0x1280041f,
	0x18c0001f, 0x10006200, 0x1212841f, 0xc0c01740, 0x12807c1f, 0xe8208000,
	0x1000625c, 0x00000000, 0x1b80001f, 0x20000080, 0xc0c01740, 0x1280041f,
	0x18c0001f, 0x10000338, 0x1910001f, 0x10000338, 0xa1108404, 0xe0c00004,
	0x19c0001f, 0x60415c20, 0x18c0001f, 0x10006b14, 0x1910001f, 0x10006b14,
	0x09000004, 0x00000001, 0xe0c00004, 0x1ac0001f, 0x55aa55aa, 0x10007c1f,
	0xf0000000
};
static struct pcm_desc sodi_pcm = {
	.version	= "pcm_sodi_v0.1_20141124",
	.base		= sodi_binary,
	.size		= 685,
	.sess		= 2,
	.replace	= 0,
	.vec0		= EVENT_VEC(30, 1, 0, 0),	/* FUNC_APSRC_WAKEUP */
	.vec1		= EVENT_VEC(31, 1, 0, 85),	/* FUNC_APSRC_SLEEP */
	.vec2		= EVENT_VEC(11, 1, 0, 150),	/* FUNC_VCORE_HIGH */
	.vec3		= EVENT_VEC(12, 1, 0, 157),	/* FUNC_VCORE_LOW */	
};
#else
//DVT FW
static const u32 sodi_binary[] = {
	0x81431801, 0xd82003c5, 0x17c07c1f, 0x81411801, 0xd8000245, 0x17c07c1f,
	0x18c0001f, 0x10006240, 0xe0e00016, 0xe0e0001e, 0xe0e0000e, 0xe0e0000f,
	0x18c0001f, 0x102135cc, 0x1910001f, 0x102135cc, 0x813f8404, 0xe0c00004,
	0x803e0400, 0x1b80001f, 0x20000222, 0x80380400, 0x1b80001f, 0x20000280,
	0x803b0400, 0x1b80001f, 0x2000001a, 0x803d0400, 0x1b80001f, 0x20000208,
	0x80340400, 0x80310400, 0x81431801, 0xd82008e5, 0x17c07c1f, 0x1b80001f,
	0x2000000a, 0x18c0001f, 0x10006240, 0xe0e0000d, 0x81411801, 0xd80008a5,
	0x17c07c1f, 0x1b80001f, 0x20000020, 0x18c0001f, 0x102130f0, 0x1910001f,
	0x102130f0, 0xa9000004, 0x10000000, 0xe0c00004, 0x1b80001f, 0x2000000a,
	0x89000004, 0xefffffff, 0xe0c00004, 0x18c0001f, 0x102140f4, 0x1910001f,
	0x102140f4, 0xa9000004, 0x02000000, 0xe0c00004, 0x1b80001f, 0x2000000a,
	0x89000004, 0xfdffffff, 0xe0c00004, 0x1b80001f, 0x20000100, 0x81fa0407,
	0x81f08407, 0xe8208000, 0x10006354, 0x00defba1, 0xa1d80407, 0xa1de8407,
	0xa1df0407, 0xc2801e20, 0x1291041f, 0x1b00001f, 0xaf7cefff, 0xf0000000,
	0x17c07c1f, 0x1a50001f, 0x10006608, 0x80c9a401, 0x810ba401, 0x10920c1f,
	0xa0979002, 0x80ca2401, 0xa0938c02, 0xa0958402, 0x8080080d, 0xd8200d62,
	0x17c07c1f, 0x1b00001f, 0x2f7ce7ff, 0x1b80001f, 0x20000004, 0xd800146c,
	0x17c07c1f, 0x1b00001f, 0xaf7cefff, 0xd800146c, 0x17c07c1f, 0x81f80407,
	0x81fe8407, 0x81ff0407, 0x1880001f, 0x10006320, 0xc0c01860, 0xe080000f,
	0xd8000c23, 0x17c07c1f, 0xe080001f, 0x1950001f, 0x10216b00, 0x81431401,
	0xd8000ea5, 0x17c07c1f, 0xa1da0407, 0xa0110400, 0x1950001f, 0x10216b00,
	0x81439401, 0xd8000f85, 0x17c07c1f, 0xa0140400, 0x1950001f, 0x10216b00,
	0x81441401, 0xd8001045, 0x17c07c1f, 0x81431801, 0xd82013e5, 0x17c07c1f,
	0xa0180400, 0xa01b0400, 0xa01d0400, 0x1b80001f, 0x20000068, 0xa01e0400,
	0x1b80001f, 0x20000104, 0x81411801, 0xd80013e5, 0x17c07c1f, 0x18c0001f,
	0x10006240, 0xc0c017c0, 0x17c07c1f, 0x18c0001f, 0x102135cc, 0x1910001f,
	0x102135cc, 0xa11f8404, 0xe0c00004, 0xc2801e20, 0x1291841f, 0x1b00001f,
	0x6f7ce7ff, 0xf0000000, 0x17c07c1f, 0x80378400, 0xc2801e20, 0x1290041f,
	0x1b00001f, 0x2f7cf7ff, 0xf0000000, 0x17c07c1f, 0x1b00001f, 0x2f7ce7ff,
	0x1b80001f, 0x20000004, 0xd800178c, 0x17c07c1f, 0xc2801e20, 0x129f041f,
	0xa0178400, 0x1950001f, 0x10216b00, 0x81429401, 0xd80016a5, 0x17c07c1f,
	0x1b00001f, 0xaf7cefff, 0xf0000000, 0x17c07c1f, 0xe0e0000f, 0xe0e0001e,
	0xe0e00012, 0xf0000000, 0x17c07c1f, 0x1112841f, 0xa1d08407, 0xd8201924,
	0x80eab401, 0xd80018a3, 0x01200404, 0x1a00001f, 0x10006814, 0xe2000003,
	0xf0000000, 0x17c07c1f, 0xd8001a8a, 0x17c07c1f, 0xe2e00036, 0xe2e0003e,
	0x1380201f, 0xe2e0003c, 0xd8201bca, 0x17c07c1f, 0x1b80001f, 0x20000018,
	0xe2e0007c, 0x1b80001f, 0x20000003, 0xe2e0005c, 0xe2e0004c, 0xe2e0004d,
	0xf0000000, 0x17c07c1f, 0xa1d10407, 0x1b80001f, 0x20000020, 0xf0000000,
	0x17c07c1f, 0xd8001d4a, 0x17c07c1f, 0xe2e0004f, 0xe2e0006f, 0xe2e0002f,
	0xd8201dea, 0x17c07c1f, 0xe2e0002e, 0xe2e0003e, 0xe2e00032, 0xf0000000,
	0x17c07c1f, 0x18c0001f, 0x10006b18, 0x1910001f, 0x10006b18, 0xa1002804,
	0xe0c00004, 0xf0000000, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x1840001f, 0x00000001, 0xa1d48407, 0x1990001f,
	0x10006b08, 0xe8208000, 0x10006b18, 0x00000000, 0x1b00001f, 0x2f7ce7ff,
	0x1b80001f, 0xd00f0000, 0x81469801, 0xd8204265, 0x17c07c1f, 0x8880000c,
	0x2f7ce7ff, 0xd8005722, 0x17c07c1f, 0xe8208000, 0x10006354, 0x00defba1,
	0xc0c01c00, 0x81401801, 0x18c0001f, 0x10000338, 0x1910001f, 0x10000338,
	0x81308404, 0xe0c00004, 0xd8004ac5, 0x17c07c1f, 0x81f60407, 0x1950001f,
	0x10216b00, 0x81401401, 0xd8004425, 0x17c07c1f, 0x18c0001f, 0x10006200,
	0xc0c01ca0, 0x12807c1f, 0xe8208000, 0x1000625c, 0x00000001, 0x1b80001f,
	0x20000080, 0xc0c01ca0, 0x1280041f, 0x18c0001f, 0x10006208, 0xc0c01ca0,
	0x12807c1f, 0x1950001f, 0x10216b00, 0x81409401, 0xd80046a5, 0x17c07c1f,
	0xe8208000, 0x10006248, 0x00000000, 0x1b80001f, 0x20000080, 0xc0c01ca0,
	0x1280041f, 0x1950001f, 0x10216b00, 0x81411401, 0xd8004825, 0x17c07c1f,
	0x18c0001f, 0x10006290, 0xc0c01ca0, 0x12807c1f, 0xc0c01ca0, 0x1280041f,
	0x1950001f, 0x10216b00, 0x81419401, 0xd8004985, 0x17c07c1f, 0x18c0001f,
	0x100062dc, 0xe0c00001, 0xc2801e20, 0x1292041f, 0x81469801, 0xd8004ba5,
	0x17c07c1f, 0x8880000c, 0x2f7ce7ff, 0xd80051c2, 0x17c07c1f, 0x18c0001f,
	0x10006294, 0xe0f07fff, 0xe0e00fff, 0xe0e000ff, 0xa1d98407, 0xa0108400,
	0xa0148400, 0xa01b8400, 0xa0188400, 0x1950001f, 0x10216b00, 0x81421401,
	0xd8004ce5, 0x17c07c1f, 0xe8208000, 0x10006310, 0x0b1600f8, 0x1b00001f,
	0x2f7cf7ff, 0x1b80001f, 0x90100000, 0x80c10001, 0xc8c00003, 0x17c07c1f,
	0x80c78001, 0xc8c014a3, 0x17c07c1f, 0x18c0001f, 0x10006294, 0xe0e001fe,
	0xe0e003fc, 0xe0e007f8, 0xe0e00ff0, 0x1b80001f, 0x20000020, 0xe0f07ff0,
	0xe0f07f00, 0x80388400, 0x1b80001f, 0x20000300, 0x803b8400, 0x1b80001f,
	0x20000300, 0x80348400, 0x1b80001f, 0x20000104, 0x10007c1f, 0x81f38407,
	0x81401801, 0xd8005665, 0x17c07c1f, 0x18c0001f, 0x100062dc, 0xe0c0001f,
	0x18c0001f, 0x10006290, 0x1212841f, 0xc0c019c0, 0x12807c1f, 0xc0c019c0,
	0x1280041f, 0x18c0001f, 0x10006208, 0x1212841f, 0xc0c019c0, 0x12807c1f,
	0xe8208000, 0x10006248, 0x00000001, 0x1b80001f, 0x20000080, 0xc0c019c0,
	0x1280041f, 0x18c0001f, 0x10006200, 0x1212841f, 0xc0c019c0, 0x12807c1f,
	0xe8208000, 0x1000625c, 0x00000000, 0x1b80001f, 0x20000080, 0xc0c019c0,
	0x1280041f, 0x18c0001f, 0x10000338, 0x1910001f, 0x10000338, 0xa1108404,
	0xe0c00004, 0x19c0001f, 0x60415c20, 0x18c0001f, 0x10006b14, 0x1910001f,
	0x10006b14, 0x09000004, 0x00000001, 0xe0c00004, 0x1ac0001f, 0x55aa55aa,
	0x10007c1f, 0xf0000000
};
static struct pcm_desc sodi_pcm = {
	.version	= "pcm_sodi_v0.2_20141124",
	.base		= sodi_binary,
	.size		= 710,
	.sess		= 2,
	.replace	= 0,
	.vec0		= EVENT_VEC(30, 1, 0, 0),	/* FUNC_APSRC_WAKEUP */
	.vec1		= EVENT_VEC(31, 1, 0, 85),	/* FUNC_APSRC_SLEEP */
	.vec2		= EVENT_VEC(11, 1, 0, 165),	/* FUNC_VCORE_HIGH */
	.vec3		= EVENT_VEC(12, 1, 0, 172),	/* FUNC_VCORE_LOW */	
};
#endif

static struct pwr_ctrl sodi_ctrl = {
	.wake_src		= WAKE_SRC_FOR_SODI,
	
	.r0_ctrl_en		= 1,
	.r7_ctrl_en		= 1,

#if (!SODI_DVT_APxGPT)
	.ca7_wfi0_en		= 1,
	.ca7_wfi1_en		= 1,
	.ca7_wfi2_en		= 1,
	.ca7_wfi3_en		= 1,
	.ca15_wfi0_en		= 1,
	.ca15_wfi1_en		= 1,
	.ca15_wfi2_en		= 1,
	.ca15_wfi3_en		= 1,
#else
	.ca7_wfi0_en		= 1,
	.ca7_wfi1_en		= 0,
	.ca7_wfi2_en		= 0,
	.ca7_wfi3_en		= 0,
	.ca15_wfi0_en		= 0,
	.ca15_wfi1_en		= 0,
	.ca15_wfi2_en		= 0,
	.ca15_wfi3_en		= 0,
#endif

	/* SPM_AP_STANBY_CON */
	.wfi_op			= WFI_OP_AND,
	.mfg_req_mask		= 1,
	.lte_mask			= 1,
#if (SODI_DVT_APxGPT)
	.ca7top_idle_mask   = 1,
	.ca15top_idle_mask  = 1,
	.mcusys_idle_mask   = 1,
	.disp_req_mask		= 1,
	.md1_req_mask		= 1,
	.md2_req_mask		= 1,
	.conn_mask			= 1,
#endif	

	/* SPM_PCM_SRC_REQ */
#if SODI_DVT_APxGPT
    .pcm_apsrc_req      = 0,
    .pcm_f26m_req       = 0,
	.ccif0_to_ap_mask   = 1,
	.ccif0_to_md_mask   = 1,
	.ccif1_to_ap_mask   = 1,
	.ccif1_to_md_mask   = 1,
	.ccifmd_md1_event_mask = 1,
	.ccifmd_md2_event_mask = 1,
#endif

#if SPM_BYPASS_SYSPWREQ
	.syspwreq_mask		= 1,
#endif

#if SODI_DVT_APxGPT && SODI_DVT_BLOCK_BF_WFI
	.pcm_reserve		= 0x000003ff, //SPM DVT test step by step (will be defined by Hank)
#endif
};

struct spm_lp_scen __spm_sodi = {
	.pcmdesc	= &sodi_pcm,
	.pwrctrl	= &sodi_ctrl,
};

static bool gSpm_SODI_mempll_pwr_mode = 1;
static bool gSpm_sodi_en=0;

extern int mt_irq_mask_all(struct mtk_irq_mask *mask);
extern int mt_irq_mask_restore(struct mtk_irq_mask *mask);
extern void mt_irq_unmask_for_sleep(unsigned int irq);

extern void soidle_before_wfi(int cpu);
extern void soidle_after_wfi(int cpu);
extern void spm_i2c_control(u32 channel, bool onoff);


#if SPM_SODI_DEBUG
static void spm_sodi_dump_regs(void)
{
    /* SPM register */
    spm_idle_ver("SPM_MP0_CPU0_IRQ_MASK   0x%x = 0x%x\n", SPM_CA7_CPU0_IRQ_MASK, spm_read(SPM_CA7_CPU0_IRQ_MASK));
    spm_idle_ver("SPM_MP0_CPU1_IRQ_MASK   0x%x = 0x%x\n", SPM_CA7_CPU1_IRQ_MASK, spm_read(SPM_CA7_CPU1_IRQ_MASK));    
    spm_idle_ver("SPM_MP0_CPU2_IRQ_MASK   0x%x = 0x%x\n", SPM_CA7_CPU2_IRQ_MASK, spm_read(SPM_CA7_CPU2_IRQ_MASK));
    spm_idle_ver("SPM_MP0_CPU3_IRQ_MASK   0x%x = 0x%x\n", SPM_CA7_CPU3_IRQ_MASK, spm_read(SPM_CA7_CPU3_IRQ_MASK));
    spm_idle_ver("SPM_MP1_CPU0_IRQ_MASK   0x%x = 0x%x\n", SPM_MP1_CPU0_IRQ_MASK, spm_read(SPM_MP1_CPU0_IRQ_MASK));
    spm_idle_ver("SPM_MP1_CPU1_IRQ_MASK   0x%x = 0x%x\n", SPM_MP1_CPU1_IRQ_MASK, spm_read(SPM_MP1_CPU1_IRQ_MASK));
    spm_idle_ver("SPM_MP1_CPU2_IRQ_MASK   0x%x = 0x%x\n", SPM_MP1_CPU2_IRQ_MASK, spm_read(SPM_MP1_CPU2_IRQ_MASK));
    spm_idle_ver("SPM_MP1_CPU3_IRQ_MASK   0x%x = 0x%x\n", SPM_MP1_CPU3_IRQ_MASK, spm_read(SPM_MP1_CPU3_IRQ_MASK));    
#if 0    
    spm_idle_ver("POWER_ON_VAL0   0x%x = 0x%x\n", SPM_POWER_ON_VAL0          , spm_read(SPM_POWER_ON_VAL0));
    spm_idle_ver("POWER_ON_VAL1   0x%x = 0x%x\n", SPM_POWER_ON_VAL1          , spm_read(SPM_POWER_ON_VAL1));
    spm_idle_ver("PCM_PWR_IO_EN   0x%x = 0x%x\n", SPM_PCM_PWR_IO_EN          , spm_read(SPM_PCM_PWR_IO_EN));
    spm_idle_ver("CLK_CON         0x%x = 0x%x\n", SPM_CLK_CON                , spm_read(SPM_CLK_CON));
    spm_idle_ver("AP_DVFS_CON     0x%x = 0x%x\n", SPM_AP_DVFS_CON_SET        , spm_read(SPM_AP_DVFS_CON_SET));
    spm_idle_ver("PWR_STATUS      0x%x = 0x%x\n", SPM_PWR_STATUS             , spm_read(SPM_PWR_STATUS));
    spm_idle_ver("PWR_STATUS_S    0x%x = 0x%x\n", SPM_PWR_STATUS_S           , spm_read(SPM_PWR_STATUS_S));
    spm_idle_ver("SLEEP_TIMER_STA 0x%x = 0x%x\n", SPM_SLEEP_TIMER_STA        , spm_read(SPM_SLEEP_TIMER_STA));
    spm_idle_ver("WAKE_EVENT_MASK 0x%x = 0x%x\n", SPM_SLEEP_WAKEUP_EVENT_MASK, spm_read(SPM_SLEEP_WAKEUP_EVENT_MASK));
    spm_idle_ver("SPM_SLEEP_CPU_WAKEUP_EVENT 0x%x = 0x%x\n", SPM_SLEEP_CPU_WAKEUP_EVENT, spm_read(SPM_SLEEP_CPU_WAKEUP_EVENT));
    spm_idle_ver("SPM_PCM_RESERVE   0x%x = 0x%x\n", SPM_PCM_RESERVE          , spm_read(SPM_PCM_RESERVE));  
    spm_idle_ver("SPM_AP_STANBY_CON   0x%x = 0x%x\n", SPM_AP_STANBY_CON          , spm_read(SPM_AP_STANBY_CON));  
    spm_idle_ver("SPM_PCM_TIMER_OUT   0x%x = 0x%x\n", SPM_PCM_TIMER_OUT          , spm_read(SPM_PCM_TIMER_OUT));
    spm_idle_ver("SPM_PCM_CON1   0x%x = 0x%x\n", SPM_PCM_CON1          , spm_read(SPM_PCM_CON1));
#endif    
    
    // PCM register
    spm_idle_ver("PCM_REG0_DATA   0x%x = 0x%x\n", SPM_PCM_REG0_DATA          , spm_read(SPM_PCM_REG0_DATA));
    spm_idle_ver("PCM_REG1_DATA   0x%x = 0x%x\n", SPM_PCM_REG1_DATA          , spm_read(SPM_PCM_REG1_DATA));
    spm_idle_ver("PCM_REG2_DATA   0x%x = 0x%x\n", SPM_PCM_REG2_DATA          , spm_read(SPM_PCM_REG2_DATA));
    spm_idle_ver("PCM_REG3_DATA   0x%x = 0x%x\n", SPM_PCM_REG3_DATA          , spm_read(SPM_PCM_REG3_DATA));
    spm_idle_ver("PCM_REG4_DATA   0x%x = 0x%x\n", SPM_PCM_REG4_DATA          , spm_read(SPM_PCM_REG4_DATA));
    spm_idle_ver("PCM_REG5_DATA   0x%x = 0x%x\n", SPM_PCM_REG5_DATA          , spm_read(SPM_PCM_REG5_DATA));
    spm_idle_ver("PCM_REG6_DATA   0x%x = 0x%x\n", SPM_PCM_REG6_DATA          , spm_read(SPM_PCM_REG6_DATA));
    spm_idle_ver("PCM_REG7_DATA   0x%x = 0x%x\n", SPM_PCM_REG7_DATA          , spm_read(SPM_PCM_REG7_DATA));
    spm_idle_ver("PCM_REG8_DATA   0x%x = 0x%x\n", SPM_PCM_REG8_DATA          , spm_read(SPM_PCM_REG8_DATA));
    spm_idle_ver("PCM_REG9_DATA   0x%x = 0x%x\n", SPM_PCM_REG9_DATA          , spm_read(SPM_PCM_REG9_DATA));
    spm_idle_ver("PCM_REG10_DATA   0x%x = 0x%x\n", SPM_PCM_REG10_DATA          , spm_read(SPM_PCM_REG10_DATA));
    spm_idle_ver("PCM_REG11_DATA   0x%x = 0x%x\n", SPM_PCM_REG11_DATA          , spm_read(SPM_PCM_REG11_DATA));
    spm_idle_ver("PCM_REG12_DATA   0x%x = 0x%x\n", SPM_PCM_REG12_DATA          , spm_read(SPM_PCM_REG12_DATA));
    spm_idle_ver("PCM_REG13_DATA   0x%x = 0x%x\n", SPM_PCM_REG13_DATA          , spm_read(SPM_PCM_REG13_DATA));
    spm_idle_ver("PCM_REG14_DATA   0x%x = 0x%x\n", SPM_PCM_REG14_DATA          , spm_read(SPM_PCM_REG14_DATA));
    spm_idle_ver("PCM_REG15_DATA   0x%x = 0x%x\n", SPM_PCM_REG15_DATA          , spm_read(SPM_PCM_REG15_DATA));  

    spm_idle_ver("SPM_MP0_FC0_PWR_CON   0x%x = 0x%x\n", SPM_MP0_FC0_PWR_CON, spm_read(SPM_MP0_FC0_PWR_CON));    
    spm_idle_ver("SPM_MP0_FC1_PWR_CON   0x%x = 0x%x\n", SPM_MP0_FC1_PWR_CON, spm_read(SPM_MP0_FC1_PWR_CON));    
    spm_idle_ver("SPM_MP0_FC2_PWR_CON   0x%x = 0x%x\n", SPM_MP0_FC2_PWR_CON, spm_read(SPM_MP0_FC2_PWR_CON));    
    spm_idle_ver("SPM_MP0_FC3_PWR_CON   0x%x = 0x%x\n", SPM_MP0_FC3_PWR_CON, spm_read(SPM_MP0_FC3_PWR_CON));    
    spm_idle_ver("SPM_MP1_FC0_PWR_CON   0x%x = 0x%x\n", SPM_MP1_FC0_PWR_CON, spm_read(SPM_MP1_FC0_PWR_CON));    
    spm_idle_ver("SPM_MP1_FC1_PWR_CON   0x%x = 0x%x\n", SPM_MP1_FC1_PWR_CON, spm_read(SPM_MP1_FC1_PWR_CON));    
    spm_idle_ver("SPM_MP1_FC2_PWR_CON   0x%x = 0x%x\n", SPM_MP1_FC2_PWR_CON, spm_read(SPM_MP1_FC2_PWR_CON));    
    spm_idle_ver("SPM_MP1_FC3_PWR_CON   0x%x = 0x%x\n", SPM_MP1_FC3_PWR_CON, spm_read(SPM_MP1_FC3_PWR_CON));    

    spm_idle_ver("CLK_CON         0x%x = 0x%x\n", SPM_CLK_CON                , spm_read(SPM_CLK_CON));
    spm_idle_ver("SPM_PCM_CON0   0x%x = 0x%x\n", SPM_PCM_CON0          , spm_read(SPM_PCM_CON0));
    spm_idle_ver("SPM_PCM_CON1   0x%x = 0x%x\n", SPM_PCM_CON1          , spm_read(SPM_PCM_CON1));
    
    spm_idle_ver("SPM_PCM_MP_CORE0_AUX   0x%x = 0x%x\n", SPM_PCM_EVENT_VECTOR2  , spm_read(SPM_PCM_EVENT_VECTOR2));
    spm_idle_ver("SPM_PCM_MP_CORE1_AUX   0x%x = 0x%x\n", SPM_PCM_EVENT_VECTOR3  , spm_read(SPM_PCM_EVENT_VECTOR3));
    spm_idle_ver("SPM_PCM_MP_CORE2_AUX   0x%x = 0x%x\n", SPM_PCM_EVENT_VECTOR4  , spm_read(SPM_PCM_EVENT_VECTOR4));
    spm_idle_ver("SPM_PCM_MP_CORE3_AUX   0x%x = 0x%x\n", SPM_PCM_EVENT_VECTOR5  , spm_read(SPM_PCM_EVENT_VECTOR5));
    spm_idle_ver("SPM_PCM_MP_CORE4_AUX   0x%x = 0x%x\n", SPM_PCM_EVENT_VECTOR6  , spm_read(SPM_PCM_EVENT_VECTOR6));
    spm_idle_ver("SPM_PCM_MP_CORE5_AUX   0x%x = 0x%x\n", SPM_PCM_EVENT_VECTOR7  , spm_read(SPM_PCM_EVENT_VECTOR7));
    spm_idle_ver("SPM_PCM_MP_CORE6_AUX   0x%x = 0x%x\n", SPM_PCM_RESERVE  , spm_read(SPM_PCM_RESERVE));
    spm_idle_ver("SPM_PCM_MP_CORE7_AUX   0x%x = 0x%x\n", SPM_PCM_WDT_TIMER_VAL  , spm_read(SPM_PCM_WDT_TIMER_VAL));

    spm_idle_ver("SPM_MP0_CORE0_WFI_SEL   0x%x = 0x%x\n", SPM_SLEEP_CA7_WFI0_EN  , spm_read(SPM_SLEEP_CA7_WFI0_EN));
    spm_idle_ver("SPM_MP0_CORE1_WFI_SEL   0x%x = 0x%x\n", SPM_SLEEP_CA7_WFI1_EN  , spm_read(SPM_SLEEP_CA7_WFI1_EN));
    spm_idle_ver("SPM_MP0_CORE2_WFI_SEL   0x%x = 0x%x\n", SPM_SLEEP_CA7_WFI2_EN  , spm_read(SPM_SLEEP_CA7_WFI2_EN));
    spm_idle_ver("SPM_MP0_CORE3_WFI_SEL   0x%x = 0x%x\n", SPM_SLEEP_CA7_WFI3_EN  , spm_read(SPM_SLEEP_CA7_WFI3_EN));
    spm_idle_ver("SPM_MP1_CORE0_WFI_SEL   0x%x = 0x%x\n", SPM_SLEEP_CA15_WFI0_EN  , spm_read(SPM_SLEEP_CA15_WFI0_EN));
    spm_idle_ver("SPM_MP1_CORE1_WFI_SEL   0x%x = 0x%x\n", SPM_SLEEP_CA15_WFI1_EN  , spm_read(SPM_SLEEP_CA15_WFI1_EN));
    spm_idle_ver("SPM_MP1_CORE2_WFI_SEL   0x%x = 0x%x\n", SPM_SLEEP_CA15_WFI2_EN  , spm_read(SPM_SLEEP_CA15_WFI2_EN));
    spm_idle_ver("SPM_MP1_CORE3_WFI_SEL   0x%x = 0x%x\n", SPM_SLEEP_CA15_WFI3_EN  , spm_read(SPM_SLEEP_CA15_WFI3_EN));

    spm_idle_ver("SPM_SLEEP_TIMER_STA   0x%x = 0x%x\n", SPM_SLEEP_TIMER_STA  , spm_read(SPM_SLEEP_TIMER_STA));
    spm_idle_ver("SPM_PWR_STATUS   0x%x = 0x%x\n", SPM_PWR_STATUS  , spm_read(SPM_PWR_STATUS));
    spm_idle_ver("SPM_PWR_STATUS_S   0x%x = 0x%x\n", SPM_PWR_STATUS_S  , spm_read(SPM_PWR_STATUS_S));

    spm_idle_ver("SPM_MP0_FC0_PWR_CON   0x%x = 0x%x\n", SPM_MP0_FC0_PWR_CON  , spm_read(SPM_MP0_FC0_PWR_CON));
    spm_idle_ver("SPM_MP0_DBG_PWR_CON   0x%x = 0x%x\n", SPM_MP0_DBG_PWR_CON  , spm_read(SPM_MP0_DBG_PWR_CON));    
    spm_idle_ver("SPM_MP0_CPU_PWR_CON   0x%x = 0x%x\n", SPM_MP0_CPU_PWR_CON  , spm_read(SPM_MP0_CPU_PWR_CON));    
 
}
#endif


static void spm_trigger_wfi_for_sodi(struct pwr_ctrl *pwrctrl)
{
    if (is_cpu_pdn(pwrctrl->pcm_flags)) {    
        mt_cpu_dormant(CPU_SODI_MODE);
    } else {
        u32 val = 0;
        
        //backup MP0_AXI_CONFIG
    	val = spm_read(MP0_AXI_CONFIG);
    	
    	//disable snoop function 
        spm_write(MP0_AXI_CONFIG, val | ACINACTM);  
        
        //enter WFI
        wfi_with_sync();
        
        //restore MP0_AXI_CONFIG
        spm_write(MP0_AXI_CONFIG, val);
    }
}

void spm_go_to_sodi(u32 spm_flags, u32 spm_data)
{
    struct wake_status wakesta;
    unsigned long flags;
    struct mtk_irq_mask mask;
    wake_reason_t wr = WR_NONE;
    struct pcm_desc *pcmdesc = __spm_sodi.pcmdesc;
    struct pwr_ctrl *pwrctrl = __spm_sodi.pwrctrl;

#if SODI_DVT_APxGPT
	spm_flags |= (SPM_CPU_PDN_DIS | SPM_MD_VRF18_DIS | SPM_VCORE_DVS_EVENT_EN | SPM_DISABLE_ATF_ABORT);
#else
    spm_flags |= (SPM_VCORE_DVFS_EN);
    
  #if defined (CONFIG_ARM_PSCI)||defined(CONFIG_MTK_PSCI)
    spm_flags &= ~SPM_DISABLE_ATF_ABORT;
  #else
    spm_flags |= SPM_DISABLE_ATF_ABORT;
  #endif
#endif

    set_pwrctrl_pcm_flags(pwrctrl, spm_flags);

    /* set PMIC WRAP table for deepidle power control */
    mt_cpufreq_set_pmic_phase(PMIC_WRAP_PHASE_DEEPIDLE);	
    
    //enable APxGPT timer
	soidle_before_wfi(0);
	
    spin_lock_irqsave(&__spm_lock, flags);

    mt_irq_mask_all(&mask);
    mt_irq_unmask_for_sleep(SPM_IRQ0_ID);
    mt_cirq_clone_gic();
    mt_cirq_enable();

    __spm_reset_and_init_pcm(pcmdesc);

#if !SODI_DVT_APxGPT
	if(gSpm_SODI_mempll_pwr_mode == 1)
	{
		pwrctrl->pcm_flags |= SPM_MEMPLL_CG_EN; //MEMPLL CG mode
	}
	else
	{
		pwrctrl->pcm_flags &= ~SPM_MEMPLL_CG_EN; //DDRPHY power down mode
	}
#else
	pwrctrl->pcm_flags |= SPM_MEMPLL_CG_EN;
#endif

	/*
	 * When commond-queue is in shut-down mode, SPM will hang if it tries to access commond-queue status.  
     * Follwoing patch is to let SODI driver to notify SPM that commond-queue is in shut-down mode or not to avoid above SPM hang issue. 
     * But, now display can automatically notify SPM that command-queue is shut-down or not, so following code is not needed anymore.
	 */
	#if 0 
    //check GCE
	if(clock_is_on(MT_CG_INFRA_GCE))
	{
		pwrctrl->pcm_flags &= ~SPM_DDR_HIGH_SPEED; 
	}
	else
	{
		pwrctrl->pcm_flags |= SPM_DDR_HIGH_SPEED; 
	}
	#endif

    __spm_kick_im_to_fetch(pcmdesc);

    __spm_init_pcm_register();

    __spm_init_event_vector(pcmdesc);

    //Display will control SPM_PCM_SRC_REQ[0] to force DRAM not enter self-refresh mode in a specifice case
	//keep bit 1's value for video/cmd mode lcm
	if((spm_read(SPM_PCM_SRC_REQ)&0x00000001))
	{
		pwrctrl->pcm_apsrc_req = 1;
	}
	else
	{
		pwrctrl->pcm_apsrc_req = 0;
	}

    __spm_set_power_control(pwrctrl);

    __spm_set_wakeup_event(pwrctrl);

#if SODI_DVT_APxGPT && SODI_DVT_BLOCK_BF_WFI
	//PCM_Timer is enable in above '__spm_set_wakeup_event(pwrctrl);', disable PCM Timer here
	spm_write(SPM_PCM_CON1 ,spm_read(SPM_PCM_CON1)&(~CON1_PCM_TIMER_EN));
#endif
	
    __spm_kick_pcm_to_run(pwrctrl);

#if SPM_SODI_DEBUG
    spm_idle_ver("============SODI Before============\n");
    spm_sodi_dump_regs(); //dump debug info
#endif
    
	spm_i2c_control(I2C_CHANNEL, 1);

    spm_trigger_wfi_for_sodi(pwrctrl);
    
	spm_i2c_control(I2C_CHANNEL, 0);

#if SPM_SODI_DEBUG
    spm_idle_ver("============SODI After=============\n");
    spm_sodi_dump_regs();//dump debug info
#endif

    __spm_get_wakeup_status(&wakesta);

	spm_idle_ver("SODI:dram-selfrefrsh cnt %d",spm_read(SPM_PCM_PASR_DPD_3));

    __spm_clean_after_wakeup();

    wr = __spm_output_wake_reason(&wakesta, pcmdesc, false);

    mt_cirq_flush();
    mt_cirq_disable();
    mt_irq_mask_restore(&mask);
	
    spin_unlock_irqrestore(&__spm_lock, flags);

    //stop APxGPT timer and enable caore0 local timer
    soidle_after_wfi(0);

     /* set PMIC WRAP table for normal power control */
    mt_cpufreq_set_pmic_phase(PMIC_WRAP_PHASE_NORMAL);  

#if SODI_DVT_SPM_MEM_RW_TEST
    {	
        static u32 magic_init = 0;

        if(magic_init == 0){
		    magic_init++;
		    printk("magicNumArray:0x%x",magicArray);
	    }

        int i =0;
    	for(i=0;i<16;i++)
    	{
    		if(magicArray[i]!=SODI_DVT_MAGIC_NUM)
    		{
    			printk("Error: sodi magic number no match!!!");
    			ASSERT(0);
    		}
    	}
    }
#endif
}

void spm_sodi_mempll_pwr_mode(bool pwr_mode)
{
	//printk("[SODI]set pwr_mode = %d\n",pwr_mode);
    gSpm_SODI_mempll_pwr_mode = pwr_mode;
}

void spm_enable_sodi(bool en)
{
    gSpm_sodi_en=en;
}

bool spm_get_sodi_en(void)
{
    return gSpm_sodi_en;
}

void spm_sodi_init(void)
{
    struct device_node *node;

    //mcucfg        
    node = of_find_compatible_node(NULL, NULL, "mediatek,MCUCFG");
    if (!node) {
        spm_err("[MCUCFG] find node failed\n");
    }
    spm_mcucfg = of_iomap(node, 0);
    if (!spm_mcucfg)
        spm_err("[MCUCFG] base failed\n");

    printk("spm_mcucfg = %p\n", spm_mcucfg);  
}

MODULE_DESCRIPTION("SPM-SODI Driver v0.1");
