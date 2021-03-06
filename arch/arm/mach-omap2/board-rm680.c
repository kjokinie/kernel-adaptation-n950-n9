/*
 * Board support file for Nokia RM-680/696.
 *
 * Copyright (C) 2010 Nokia
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/i2c/twl.h>
#include <linux/platform_device.h>
#include <linux/omapfb.h>
#include <linux/regulator/fixed.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/consumer.h>

#include <asm/mach/arch.h>
#include <asm/mach-types.h>

#include <plat/i2c.h>
#include <plat/mmc.h>
#include <plat/usb.h>
#include <plat/gpmc.h>
#include "common.h"
#include <plat/onenand.h>
#include <plat/display.h>
#include <plat/panel-nokia-dsi.h>
#include <plat/vram.h>

#include "mux.h"
#include "hsmmc.h"
#include "sdram-nokia.h"
#include "common-board-devices.h"

#include "dss.h"

static struct regulator_consumer_supply rm680_vemmc_consumers[] = {
	REGULATOR_SUPPLY("vmmc", "omap_hsmmc.1"),
};

/* Fixed regulator for internal eMMC */
static struct regulator_init_data rm680_vemmc = {
	.constraints =	{
		.name			= "rm680_vemmc",
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_STATUS
					| REGULATOR_CHANGE_MODE,
	},
	.num_consumer_supplies		= ARRAY_SIZE(rm680_vemmc_consumers),
	.consumer_supplies		= rm680_vemmc_consumers,
};

static struct fixed_voltage_config rm680_vemmc_config = {
	.supply_name		= "VEMMC",
	.microvolts		= 2900000,
	.gpio			= 157,
	.startup_delay		= 150,
	.enable_high		= 1,
	.init_data		= &rm680_vemmc,
};

static struct platform_device rm680_vemmc_device = {
	.name			= "reg-fixed-voltage",
	.dev			= {
		.platform_data	= &rm680_vemmc_config,
	},
};

static struct platform_device *rm680_peripherals_devices[] __initdata = {
	&rm680_vemmc_device,
};

/* TWL */
static struct twl4030_gpio_platform_data rm680_gpio_data = {
	.gpio_base		= OMAP_MAX_GPIO_LINES,
	.irq_base		= TWL4030_GPIO_IRQ_BASE,
	.irq_end		= TWL4030_GPIO_IRQ_END,
	.pullups		= BIT(0),
	.pulldowns		= BIT(1) | BIT(2) | BIT(8) | BIT(15),
};

static struct regulator_consumer_supply rm696_vio_consumers[] = {
	REGULATOR_SUPPLY("VDDI", "display0"),	/* Himalaya */
};

static struct regulator_init_data rm696_vio_data = {
	.constraints =	{
		.name			= "rm696_vio",
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_STATUS
					| REGULATOR_CHANGE_MODE,
	},
	.num_consumer_supplies		= ARRAY_SIZE(rm696_vio_consumers),
	.consumer_supplies		= rm696_vio_consumers,
};

/*
 * According to public N9 schematics VPNL comes from battery not from
 * TWL MMC2
 */
static struct regulator_consumer_supply rm696_vmmc2_consumers[] = {
	REGULATOR_SUPPLY("VPNL", "display0"),	/* Himalaya */
};

static struct regulator_init_data rm696_vmmc2_data = {
	.constraints =	{
		.name			= "rm696_vmmc2",
		.min_uV			= 3000000,
		.max_uV			= 3000000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_STATUS
					| REGULATOR_CHANGE_MODE,
	},
	.num_consumer_supplies		= ARRAY_SIZE(rm696_vmmc2_consumers),
	.consumer_supplies		= rm696_vmmc2_consumers,
};

static struct twl4030_platform_data rm680_twl_data = {
	.gpio			= &rm680_gpio_data,
	/* add rest of the children here */
	/* LDOs */
	.vio			= &rm696_vio_data,
	.vmmc2			= &rm696_vmmc2_data,
};

static void __init rm680_i2c_init(void)
{
	omap3_pmic_get_config(&rm680_twl_data, TWL_COMMON_PDATA_USB,
			      TWL_COMMON_REGULATOR_VPLL2);
	omap_pmic_init(1, 2900, "twl5031", INT_34XX_SYS_NIRQ, &rm680_twl_data);
	omap_register_i2c_bus(2, 400, NULL, 0);
	omap_register_i2c_bus(3, 400, NULL, 0);
}

#if defined(CONFIG_MTD_ONENAND_OMAP2) || \
	defined(CONFIG_MTD_ONENAND_OMAP2_MODULE)
static struct omap_onenand_platform_data board_onenand_data[] = {
	{
		.gpio_irq	= 65,
		.flags		= ONENAND_SYNC_READWRITE,
	}
};
#endif

/* eMMC */
static struct omap2_hsmmc_info mmc[] __initdata = {
	{
		.name		= "internal",
		.mmc		= 2,
		.caps		= MMC_CAP_4_BIT_DATA | MMC_CAP_MMC_HIGHSPEED,
		.gpio_cd	= -EINVAL,
		.gpio_wp	= -EINVAL,
	},
	{ /* Terminator */ }
};

static struct nokia_dsi_panel_data rm696_panel_data = {
	.name = "pyrenees",
	.reset_gpio = 87,
	.use_ext_te = true,
	.ext_te_gpio = 62,
	.esd_timeout = 5000,
	.ulps_timeout = 500,
	.partial_area = {
		.offset = 0,
		.height = 854,
	},
	.rotate = 1,
};

static struct omap_dss_device rm696_dsi_display_data = {
	.type = OMAP_DISPLAY_TYPE_DSI,
	.name = "lcd",
	.driver_name = "panel-nokia-dsi",
	.phy.dsi = {
		.clk_lane = 2,
		.clk_pol = 0,
		.data1_lane = 3,
		.data1_pol = 0,
		.data2_lane = 1,
		.data2_pol = 0,
		.ext_te = true,
		.ext_te_gpio = 62,
	},

	.clocks = {
		.dss = {
			.fck_div = 5,
		},

		.dispc = {
			/* LCK 170.88 MHz */
			.lck_div = 1,
			/* PCK 42.72 MHz */
			.pck_div = 4,

			.fclk_from_dsi_pll = false,
		},

		.dsi = {
			/* DDR CLK 210.24 MHz */
			.regn = 10,
			.regm = 219,
			/* DISPC FCLK 170.88 MHz */
			.regm3 = 6,
			/* DSI FCLK 170.88 MHz */
			.regm4 = 6,

			/* LP CLK 8.760 MHz */
			.lp_clk_div = 8,

			.fclk_from_dsi_pll = false,
		},
	},

	.data = &rm696_panel_data,
};

static struct omap_dss_device *rm696_dss_devices[] = {
	&rm696_dsi_display_data,
};

static struct omap_dss_board_info rm696_dss_data = {
	.num_devices = ARRAY_SIZE(rm696_dss_devices),
	.devices = rm696_dss_devices,
	.default_device = &rm696_dsi_display_data,
};

struct platform_device rm696_dss_device = {
	.name          = "omapdss",
	.id            = -1,
	.dev            = {
		.platform_data = &rm696_dss_data,
	},
};

static struct omapfb_platform_data rm696_omapfb_data = {
	.mem_desc = {
		.region_cnt = 1,
		.region[0] = {
			.format_used = true,
			.format = OMAPFB_COLOR_RGB565,
			.size = PAGE_ALIGN(856 * 512 * 2 * 3),
			.xres_virtual = 856,
			.yres_virtual = 512 * 3,
		}
	}
};

static int __init rm696_video_init(void)
{
	int r;

	omap_setup_dss_device(&rm696_dss_device);

	rm696_dss_devices[0] = &rm696_dsi_display_data;

	r = gpio_request(rm696_panel_data.reset_gpio, "pyrenees reset");
	if (r < 0)
		goto err0;

	r = gpio_direction_output(rm696_panel_data.reset_gpio, 1);

	rm696_dss_data.default_device = rm696_dss_devices[0];

	r = platform_device_register(&rm696_dss_device);
	if (r < 0)
		goto err1;

	omapfb_set_platform_data(&rm696_omapfb_data);

	return 0;

err1:
	gpio_free(rm696_panel_data.reset_gpio);
	rm696_panel_data.reset_gpio = -1;
err0:
	pr_err("%s failed (%d)\n", __func__, r);

	return r;
}

subsys_initcall(rm696_video_init);

static void __init rm680_peripherals_init(void)
{
	platform_add_devices(rm680_peripherals_devices,
				ARRAY_SIZE(rm680_peripherals_devices));
	rm680_i2c_init();
	gpmc_onenand_init(board_onenand_data);
	omap_hsmmc_init(mmc);
}

#ifdef CONFIG_OMAP_MUX
static struct omap_board_mux board_mux[] __initdata = {
	{ .reg_offset = OMAP_MUX_TERMINATOR },
};
#endif

static void __init rm680_init(void)
{
	struct omap_sdrc_params *sdrc_params;

	omap3_mux_init(board_mux, OMAP_PACKAGE_CBB);
	omap_serial_init();

	sdrc_params = nokia_get_sdram_timings();
	omap_sdrc_init(sdrc_params, sdrc_params);

	usb_musb_init(NULL);
	rm680_peripherals_init();
}

static void __init rx680_reserve(void)
{
	omap_vram_set_sdram_vram(PAGE_ALIGN(856 * 512 * 2 * 3) +
			PAGE_ALIGN(1280 * 720 * 2 * 6), 0);
	omap_reserve();
}

MACHINE_START(NOKIA_RM680, "Nokia RM-680 board")
	.atag_offset	= 0x100,
	.reserve	= rx680_reserve,
	.map_io		= omap3_map_io,
	.init_early	= omap3630_init_early,
	.init_irq	= omap3_init_irq,
	.handle_irq	= omap3_intc_handle_irq,
	.init_machine	= rm680_init,
	.init_late	= omap3630_init_late,
	.timer		= &omap3_timer,
	.restart	= omap_prcm_restart,
MACHINE_END

MACHINE_START(NOKIA_RM696, "Nokia RM-696 board")
	.atag_offset	= 0x100,
	.reserve	= rx680_reserve,
	.map_io		= omap3_map_io,
	.init_early	= omap3630_init_early,
	.init_irq	= omap3_init_irq,
	.handle_irq	= omap3_intc_handle_irq,
	.init_machine	= rm680_init,
	.init_late	= omap3630_init_late,
	.timer		= &omap3_timer,
	.restart	= omap_prcm_restart,
MACHINE_END
