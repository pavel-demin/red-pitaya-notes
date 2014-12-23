# 'make' builds everything
# 'make clean' deletes everything except source files and Makefile
#
# You need to set NAME, PART, PROC and REPO for your project.
# NAME is the base name for most of the generated files.

NAME = red_pitaya
PART = xc7z010clg400-1
PROC = ps7_cortexa9_0

VIVADO = vivado -nolog -nojournal -mode tcl
HSI = hsi -nolog -nojournal -mode tcl
RM = rm -rf

UBOOT_TAG = xilinx-v2014.3
LINUX_TAG = xilinx-v2014.3
DTREE_TAG = xilinx-v2014.3

UBOOT_DIR = u-boot-xlnx-$(UBOOT_TAG)
LINUX_DIR = linux-xlnx-$(LINUX_TAG)
DTREE_DIR = device-tree-xlnx-$(DTREE_TAG)

UBOOT_TAR = u-boot-xlnx-$(UBOOT_TAG).tar.gz
LINUX_TAR = linux-xlnx-$(LINUX_TAG).tar.gz
DTREE_TAR = device-tree-xlnx-$(DTREE_TAG).tar.gz

UBOOT_URL = https://github.com/Xilinx/u-boot-xlnx/archive/$(UBOOT_TAG).tar.gz
LINUX_URL = https://github.com/Xilinx/linux-xlnx/archive/$(LINUX_TAG).tar.gz
DTREE_URL = https://github.com/Xilinx/device-tree-xlnx/archive/$(DTREE_TAG).tar.gz

LINUX_CFLAGS = "-O2 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard"
UBOOT_CFLAGS = "-O2 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard"

.PRECIOUS: %.xpr %.hwdef %.bit %.elf %.dts

all: u-boot.elf boot.bin devicetree.dtb

$(UBOOT_TAR):
	curl -L $(UBOOT_URL) -o $@

$(LINUX_TAR):
	curl -L $(LINUX_URL) -o $@

$(DTREE_TAR):
	curl -L $(DTREE_URL) -o $@

$(UBOOT_DIR): $(UBOOT_TAR)
	mkdir $@
	tar zxf $< --strip-components=1 --directory=$@
	patch -p 0 < patches/$@.patch
	cp patches/zynq_red_pitaya.h $@/include/configs
	cp patches/u-boot-lantiq.c $@/drivers/net/phy/lantiq.c

$(LINUX_DIR): $(LINUX_TAR)
	mkdir $@
	tar zxf $< --strip-components=1 --directory=$@
	patch -p 0 < patches/$@.patch
	cp patches/linux-lantiq.c $@/drivers/net/phy/lantiq.c

$(DTREE_DIR): $(DTREE_TAR)
	mkdir $@
	tar zxf $< --strip-components=1 --directory=$@

uImage: $(LINUX_DIR)
	make -C $< mrproper
	make -C $< ARCH=arm xilinx_zynq_defconfig
	make -C $< ARCH=arm CFLAGS=$(LINUX_CFLAGS) \
	  -j $(shell grep -c ^processor /proc/cpuinfo) \
	  CROSS_COMPILE=arm-linux-gnueabihf- UIMAGE_LOADADDR=0x8000 uImage
	cp $</arch/arm/boot/uImage $@

u-boot.elf: $(UBOOT_DIR)
	make -C $< arch=ARM zynq_red_pitaya_config
	make -C $< arch=ARM CFLAGS=$(UBOOT_CFLAGS) \
	  CROSS_COMPILE=arm-linux-gnueabihf- all
	cp $</u-boot $@

# fw_printenv: $(UBOOT_DIR) u-boot.elf
#	make -C $< arch=ARM CFLAGS=$(UBOOT_CFLAGS) \
#	  CROSS_COMPILE=arm-linux-gnueabihf- env
#	cp $</tools/env/fw_printenv $@

# rootfs.tar.gz: fw_printenv
rootfs.tar.gz:
	su -c 'sudo sh scripts/rootfs.sh'

boot.bin: $(NAME).elf $(NAME).bit u-boot.elf
	echo "img:{[bootloader] $^}" > boot.bif
	bootgen -image boot.bif -w -o i $@

devicetree.dtb: uImage $(NAME).dts
	$(LINUX_DIR)/scripts/dtc/dtc -I dts -O dtb -o devicetree.dtb $(NAME).dts

%.xpr: %
	$(RM) $@ $*.cache $*.srcs $*.runs
	$(VIVADO) -source scripts/project.tcl -tclargs $* $(PART)

%.hwdef: %.xpr
	$(VIVADO) -source scripts/hwdef.tcl -tclargs $*

%.bit: %.xpr
	$(VIVADO) -source scripts/bitstream.tcl -tclargs $*

%.elf: %.hwdef
	$(HSI) -source scripts/fsbl.tcl -tclargs $* $(PROC)

%.dts: %.hwdef $(DTREE_DIR)
	$(HSI) -source scripts/devicetree.tcl -tclargs $* $(PROC) $(DTREE_DIR)
	patch $*.dts patches/devicetree.patch

clean:
	$(RM) $(UBOOT_DIR) $(UBOOT_TAR)
	$(RM) $(LINUX_DIR) $(LINUX_TAR)
	$(RM) $(DTREE_DIR) $(DTREE_TAR)
	$(RM) uImage u-boot.elf fw_printenv boot.bif boot.bin devicetree.dtb
	$(RM) ps.dtsi $(NAME).dts $(NAME).elf
	$(RM) $(NAME).bit $(NAME).hwdef $(NAME).xpr
	$(RM) $(NAME).cache $(NAME).srcs $(NAME).runs tmp .Xil
	$(RM) usage_statistics_webtalk.html usage_statistics_webtalk.xml

