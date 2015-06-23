#include <linux/ioport.h>
//#include <kernel/drivers/gpu/arm/midgard/common/mali_kbase.h>
//#include <kernel/drivers/gpu/arm/midgard/common/mali_kbase_defs.h>
//#include <kernel/drivers/gpu/arm/midgard/linux/mali_kbase_config_linux.h>
#include "mali_kbase.h"
#include "mali_kbase_defs.h"
#include "mali_kbase_config.h"
/*
SYSCIRQ Interrupt ID  IRQS_Sync ID  GIC Interrupt ID  Subsys IRQ Bus
153                   225           257               mali_irq_gpu_b
154                   226           258               mali_irq_mmu_b
155                   227           259               mal_irq_job_b


Module      Start Address End Address DW  Size  Software_BASE ID
G3D_CONFIG  0x13000000    0x13000FFF  32  4KB   G3D_CONFIG_BASE
MALI        0x13040000    0x13043FFF  32  16KB  MALI_BASE
*/

static struct kbase_io_resources io_resources =
{
	.job_irq_number = 227 /* e.g. 68 */,
	.mmu_irq_number = 226,
	.gpu_irq_number = 225,
	.io_memory_region =
	{
		.start = 0x13040000,
		.end = 0x13043FFF + (4096 * 4) - 1
	}
};

static struct kbase_attribute config_attributes[] = {
	{
		KBASE_CONFIG_ATTR_GPU_FREQ_KHZ_MAX,
		400000
	},
	{
		KBASE_CONFIG_ATTR_GPU_FREQ_KHZ_MIN,
		100000
	},
	{
		KBASE_CONFIG_ATTR_END,
		0
	}
};

kbase_platform_config platform_config =
{
	.attributes = config_attributes,
	.io_resources = &io_resources,
};
