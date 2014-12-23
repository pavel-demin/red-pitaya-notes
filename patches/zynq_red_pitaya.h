#ifndef __CONFIG_ZYNQ_RED_PITAYA_H
#define __CONFIG_ZYNQ_RED_PITAYA_H

#define CONFIG_SYS_SDRAM_SIZE		(512 * 1024 * 1024)

#define CONFIG_ZYNQ_SERIAL_UART0
#define CONFIG_ZYNQ_GEM0
#define CONFIG_ZYNQ_GEM_PHY_ADDR0	1

#define CONFIG_SYS_NO_FLASH

#define CONFIG_ZYNQ_USB
#define CONFIG_ZYNQ_SDHCI0

#define CONFIG_ZYNQ_BOOT_FREEBSD

#include <configs/zynq-common.h>

#undef CONFIG_PHY_MARVELL

#undef CONFIG_EXTRA_ENV_SETTINGS

#define CONFIG_PHY_LANTIQ

#define CONFIG_EXTRA_ENV_SETTINGS \
	"ethaddr=00:26:33:14:50:00\0" \
	"kernel_image=uImage\0" \
	"kernel_load_address=0x2080000\0" \
	"ramdisk_image=uramdisk.image.gz\0" \
	"ramdisk_load_address=0x4000000\0" \
	"devicetree_image=devicetree.dtb\0" \
	"devicetree_load_address=0x2000000\0" \
	"loadbootenv_addr=0x2000000\0" \
	"fdt_high=0x20000000\0" \
	"initrd_high=0x20000000\0" \
	"bootenv=uEnv.txt\0" \
	"loadbootenv=fatload mmc 0 ${loadbootenv_addr} ${bootenv}\0" \
	"importbootenv=echo Importing environment from SD ...; " \
		"env import -t ${loadbootenv_addr} $filesize\0" \
	"uenvboot=" \
		"if run loadbootenv; then " \
			"echo Loaded environment from ${bootenv}; " \
			"run importbootenv; " \
		"fi; " \
		"if test -n $uenvcmd; then " \
			"echo Running uenvcmd ...; " \
			"run uenvcmd; " \
		"fi\0" \
	"sdboot=if mmcinfo; then " \
			"run uenvboot; " \
			"echo Copying Linux from SD to RAM... && " \
			"fatload mmc 0 ${kernel_load_address} ${kernel_image} && " \
			"fatload mmc 0 ${devicetree_load_address} ${devicetree_image} && " \
			"fatload mmc 0 ${ramdisk_load_address} ${ramdisk_image} && " \
			"bootm ${kernel_load_address} ${ramdisk_load_address} ${devicetree_load_address}; " \
		"fi\0" \
		DFU_ALT_INFO

#endif /* __CONFIG_ZYNQ_RED_PITAYA_H */

