# 'make' builds everything
# 'make clean' deletes everything except source files and Makefile
#
# You need to set NAME, PART and PROC for your project.
# NAME is the base name for most of the generated files.

# solves problem with awk while building linux kernel
# solution taken from http://www.googoolia.com/wp/2015/04/21/awk-symbol-lookup-error-awk-undefined-symbol-mpfr_z_sub/
LD_LIBRARY_PATH =

NAME = led_blinker
PART = xc7z010clg400-1
PROC = ps7_cortexa9_0

FILES = $(wildcard cores/*.v)
CORES = $(FILES:.v=)

VIVADO = vivado -nolog -nojournal -mode batch
XSCT = xsct
RM = rm -rf

UBOOT_TAG = 2021.04
LINUX_TAG = 6.1
DTREE_TAG = xilinx_v2023.1

UBOOT_DIR = tmp/u-boot-$(UBOOT_TAG)
LINUX_DIR = tmp/linux-$(LINUX_TAG)
DTREE_DIR = tmp/device-tree-xlnx-$(DTREE_TAG)

UBOOT_TAR = tmp/u-boot-$(UBOOT_TAG).tar.bz2
LINUX_TAR = tmp/linux-$(LINUX_TAG).tar.xz
DTREE_TAR = tmp/device-tree-xlnx-$(DTREE_TAG).tar.gz

UBOOT_URL = https://ftp.denx.de/pub/u-boot/u-boot-$(UBOOT_TAG).tar.bz2
LINUX_URL = https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-$(LINUX_TAG).32.tar.xz
DTREE_URL = https://github.com/Xilinx/device-tree-xlnx/archive/$(DTREE_TAG).tar.gz

RTL8188_TAR = tmp/rtl8188eu-v5.2.2.4.tar.gz
RTL8188_URL = https://github.com/lwfinger/rtl8188eu/archive/v5.2.2.4.tar.gz

.PRECIOUS: tmp/cores/% tmp/%.xpr tmp/%.xsa tmp/%.bit tmp/%.fsbl/executable.elf tmp/%.tree/system-top.dts

all: tmp/$(NAME).bit boot.bin uImage devicetree.dtb

cores: $(addprefix tmp/, $(CORES))

xpr: tmp/$(NAME).xpr

bit: tmp/$(NAME).bit

$(UBOOT_TAR):
	mkdir -p $(@D)
	curl -L $(UBOOT_URL) -o $@

$(LINUX_TAR):
	mkdir -p $(@D)
	curl -L $(LINUX_URL) -o $@

$(DTREE_TAR):
	mkdir -p $(@D)
	curl -L $(DTREE_URL) -o $@

$(RTL8188_TAR):
	mkdir -p $(@D)
	curl -L $(RTL8188_URL) -o $@

$(UBOOT_DIR): $(UBOOT_TAR)
	mkdir -p $@
	tar -jxf $< --strip-components=1 --directory=$@
	patch -d tmp -p 0 < patches/u-boot-$(UBOOT_TAG).patch
	cp patches/zynq_red_pitaya_defconfig $@/configs
	cp patches/zynq-red-pitaya.dts $@/arch/arm/dts

$(LINUX_DIR): $(LINUX_TAR) $(RTL8188_TAR)
	mkdir -p $@
	tar -Jxf $< --strip-components=1 --directory=$@
	mkdir -p $@/drivers/net/wireless/realtek/rtl8188eu
	tar -zxf $(RTL8188_TAR) --strip-components=1 --directory=$@/drivers/net/wireless/realtek/rtl8188eu
	patch -d tmp -p 0 < patches/linux-$(LINUX_TAG).patch
	cp patches/zynq_ocm.c $@/arch/arm/mach-zynq
	cp patches/cma.c $@/drivers/char
	cp patches/xilinx_devcfg.c $@/drivers/char
	cp patches/xilinx_zynq_defconfig $@/arch/arm/configs

$(DTREE_DIR): $(DTREE_TAR)
	mkdir -p $@
	tar -zxf $< --strip-components=1 --directory=$@

uImage: $(LINUX_DIR)
	make -C $< mrproper
	make -C $< ARCH=arm -j $(shell nproc 2> /dev/null || echo 1) \
	  CROSS_COMPILE=arm-linux-gnueabihf- UIMAGE_LOADADDR=0x8000 \
	  xilinx_zynq_defconfig uImage modules
	cp $</arch/arm/boot/uImage $@

$(UBOOT_DIR)/u-boot.bin: $(UBOOT_DIR)
	mkdir -p $(@D)
	make -C $< mrproper
	make -C $< ARCH=arm zynq_red_pitaya_defconfig
	make -C $< ARCH=arm -j $(shell nproc 2> /dev/null || echo 1) \
	  CROSS_COMPILE=arm-linux-gnueabihf- all

boot.bin: tmp/$(NAME).fsbl/executable.elf $(UBOOT_DIR)/u-boot.bin
	echo "img:{[bootloader] tmp/$(NAME).fsbl/executable.elf [load=0x4000000,startup=0x4000000] $(UBOOT_DIR)/u-boot.bin}" > tmp/boot.bif
	bootgen -image tmp/boot.bif -w -o i $@

devicetree.dtb: uImage tmp/$(NAME).tree/system-top.dts
	$(LINUX_DIR)/scripts/dtc/dtc -I dts -O dtb -o devicetree.dtb \
	  -i tmp/$(NAME).tree tmp/$(NAME).tree/system-top.dts

tmp/cores/%: cores/%.v
	mkdir -p $(@D)
	$(VIVADO) -source scripts/core.tcl -tclargs $* $(PART)

tmp/%.xpr: projects/% $(addprefix tmp/, $(CORES))
	mkdir -p $(@D)
	$(VIVADO) -source scripts/project.tcl -tclargs $* $(PART)

tmp/%.xsa: tmp/%.xpr
	mkdir -p $(@D)
	$(VIVADO) -source scripts/hwdef.tcl -tclargs $*

tmp/%.bit: tmp/%.xpr
	mkdir -p $(@D)
	$(VIVADO) -source scripts/bitstream.tcl -tclargs $*

tmp/%.fsbl/executable.elf: tmp/%.xsa
	mkdir -p $(@D)
	$(XSCT) scripts/fsbl.tcl $* $(PROC)

tmp/%.tree/system-top.dts: tmp/%.xsa $(DTREE_DIR)
	mkdir -p $(@D)
	$(XSCT) scripts/devicetree.tcl $* $(PROC) $(DTREE_DIR)
	sed -i 's|#include|/include/|' $@
	patch -d $(@D) < patches/devicetree.patch

clean:
	$(RM) uImage boot.bin devicetree.dtb tmp
	$(RM) .Xil usage_statistics_webtalk.html usage_statistics_webtalk.xml
	$(RM) vivado*.jou vivado*.log
	$(RM) webtalk*.jou webtalk*.log
