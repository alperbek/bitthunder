/**
 *	This file describes the SDIO device on the Zynq platform.
 *
 **/
#include <bitthunder.h>

#include "slcr.h"
//#include "sdio.h"

static const BT_RESOURCE oZynq_sdio_resources_0[] = {
	{
		.ulStart 			= 0xE0100000,
		.ulEnd 				= 0xE0100000 + BT_SIZE_4K - 1,
		.ulFlags 			= BT_RESOURCE_MEM,
	},
	{
		.ulStart			= 56,
		.ulEnd				= 56,
		.ulFlags			= BT_RESOURCE_IRQ,
	},
};

static const BT_RESOURCE oZynq_sdio_resources_1[] = {
	{
		.ulStart 			= 0xE0101000,
		.ulEnd 				= 0xE0101000 + BT_SIZE_4K - 1,
		.ulFlags 			= BT_RESOURCE_MEM,
	},
	{
		.ulStart			= 79,
		.ulEnd				= 79,
		.ulFlags			= BT_RESOURCE_IRQ,
	},
};

BT_INTEGRATED_DEVICE_DEF oZynq_sdio_device_0 = {
	.name				= "mmc,sdhci",
	.ulTotalResources	= BT_ARRAY_SIZE(oZynq_sdio_resources_0),
	.pResources			= oZynq_sdio_resources_0,
};

BT_INTEGRATED_DEVICE_DEF oZynq_sdio_device_1 = {
	.name				= "mmc,sdhci",
	.ulTotalResources	= BT_ARRAY_SIZE(oZynq_sdio_resources_1),
	.pResources			= oZynq_sdio_resources_1,
};
