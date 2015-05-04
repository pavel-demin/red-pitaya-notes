# 'make' builds everything
# 'make clean' deletes everything except source files and Makefile
#
# You need to set NAME, PART, PROC and REPO for your project.
# NAME is the base name for most of the generated files.

NAME = led_blinker
PART = xc7z010clg400-1
PROC = ps7_cortexa9_0

VIVADO = vivado -nolog -nojournal -mode batch
HSI = hsi -nolog -nojournal -mode batch
RM = rm -rf

UBOOT_TAG = xilinx-v2015.1
LINUX_TAG = xilinx-v2015.1
DTREE_TAG = xilinx-v2015.1

UBOOT_DIR = tmp/u-boot-xlnx-$(UBOOT_TAG)
LINUX_DIR = tmp/linux-xlnx-$(LINUX_TAG)
DTREE_DIR = tmp/device-tree-xlnx-$(DTREE_TAG)

UBOOT_TAR = tmp/u-boot-xlnx-$(UBOOT_TAG).tar.gz
LINUX_TAR = tmp/linux-xlnx-$(LINUX_TAG).tar.gz
DTREE_TAR = tmp/device-tree-xlnx-$(DTREE_TAG).tar.gz

UBOOT_URL = https://github.com/Xilinx/u-boot-xlnx/archive/$(UBOOT_TAG).tar.gz
LINUX_URL = https://github.com/Xilinx/linux-xlnx/archive/$(LINUX_TAG).tar.gz
DTREE_URL = https://github.com/Xilinx/device-tree-xlnx/archive/$(DTREE_TAG).tar.gz

LINUX_CFLAGS = "-O2 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=softfp"
UBOOT_CFLAGS = "-O2 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=softfp"

.PRECIOUS: tmp/%.xpr tmp/%.hwdef tmp/%.bit tmp/%.fsbl/executable.elf tmp/%.tree/system.dts

all: boot.bin uImage devicetree.dtb

$(UBOOT_TAR):
	mkdir -p $(@D)
	curl -L $(UBOOT_URL) -o $@

$(LINUX_TAR):
	mkdir -p $(@D)
	curl -L $(LINUX_URL) -o $@

$(DTREE_TAR):
	mkdir -p $(@D)
	curl -L $(DTREE_URL) -o $@

$(UBOOT_DIR): $(UBOOT_TAR)
	mkdir -p $@
	tar -zxf $< --strip-components=1 --directory=$@
	patch -d tmp -p 0 < patches/u-boot-xlnx-$(UBOOT_TAG).patch
	cp patches/zynq_red_pitaya_defconfig $@/configs
	cp patches/zynq-red-pitaya.dts $@/arch/arm/dts
	cp patches/zynq_red_pitaya.h $@/include/configs
	cp patches/u-boot-lantiq.c $@/drivers/net/phy/lantiq.c

$(LINUX_DIR): $(LINUX_TAR)
	mkdir -p $@
	tar -zxf $< --strip-components=1 --directory=$@
	patch -d tmp -p 0 < patches/linux-xlnx-$(LINUX_TAG).patch
	cp patches/linux-lantiq.c $@/drivers/net/phy/lantiq.c

$(DTREE_DIR): $(DTREE_TAR)
	mkdir -p $@
	tar -zxf $< --strip-components=1 --directory=$@

uImage: $(LINUX_DIR)
	make -C $< mrproper
	make -C $< ARCH=arm xilinx_zynq_defconfig
	make -C $< ARCH=arm CFLAGS=$(LINUX_CFLAGS) \
	  -j $(shell grep -c ^processor /proc/cpuinfo) \
	  CROSS_COMPILE=arm-xilinx-linux-gnueabi- UIMAGE_LOADADDR=0x8000 uImage
	cp $</arch/arm/boot/uImage $@

tmp/u-boot.elf: $(UBOOT_DIR)
	mkdir -p $(@D)
	make -C $< arch=ARM zynq_red_pitaya_config
	make -C $< arch=ARM CFLAGS=$(UBOOT_CFLAGS) \
	  CROSS_COMPILE=arm-xilinx-linux-gnueabi- all env
	cp $</u-boot $@
	cp $</tools/env/fw_printenv fw_printenv

boot.bin: tmp/$(NAME).fsbl/executable.elf tmp/$(NAME).bit tmp/u-boot.elf
	echo "img:{[bootloader] $^}" > tmp/boot.bif
	bootgen -image tmp/boot.bif -w -o i $@

devicetree.dtb: uImage tmp/$(NAME).tree/system.dts
	$(LINUX_DIR)/scripts/dtc/dtc -I dts -O dtb -o devicetree.dtb \
	  -i tmp/$(NAME).tree tmp/$(NAME).tree/system.dts

tmp/cores:
	mkdir -p $@
	$(VIVADO) -source scripts/core.tcl -tclargs axis_red_pitaya_adc_v1_0 $(PART)
	$(VIVADO) -source scripts/core.tcl -tclargs axis_red_pitaya_dac_v1_0 $(PART)
	$(VIVADO) -source scripts/core.tcl -tclargs axis_packetizer_v1_0 $(PART)
	$(VIVADO) -source scripts/core.tcl -tclargs axis_ram_writer_v1_0 $(PART)
	$(VIVADO) -source scripts/core.tcl -tclargs axis_constant_v1_0 $(PART)
	$(VIVADO) -source scripts/core.tcl -tclargs axis_counter_v1_0 $(PART)
	$(VIVADO) -source scripts/core.tcl -tclargs axis_phase_generator_v1_0 $(PART)
	$(VIVADO) -source scripts/core.tcl -tclargs axis_bram_reader_v1_0 $(PART)
	$(VIVADO) -source scripts/core.tcl -tclargs axis_bram_writer_v1_0 $(PART)
	$(VIVADO) -source scripts/core.tcl -tclargs axis_pulse_height_analyzer_v1_0 $(PART)
	$(VIVADO) -source scripts/core.tcl -tclargs axis_histogram_v1_0 $(PART)
	$(VIVADO) -source scripts/core.tcl -tclargs axi_cfg_register_v1_0 $(PART)
	$(VIVADO) -source scripts/core.tcl -tclargs axi_sts_register_v1_0 $(PART)
	$(VIVADO) -source scripts/core.tcl -tclargs axi_bram_reader_v1_0 $(PART)
	$(VIVADO) -source scripts/core.tcl -tclargs axi_bram_writer_v1_0 $(PART)

tmp/%.xpr: projects/% tmp/cores
	mkdir -p $(@D)
	$(RM) $@ tmp/$*.cache tmp/$*.hw tmp/$*.srcs tmp/$*.runs
	$(VIVADO) -source scripts/project.tcl -tclargs $* $(PART)

tmp/%.hwdef: tmp/%.xpr
	mkdir -p $(@D)
	$(VIVADO) -source scripts/hwdef.tcl -tclargs $*

tmp/%.bit: tmp/%.xpr
	mkdir -p $(@D)
	$(VIVADO) -source scripts/bitstream.tcl -tclargs $*

tmp/%.fsbl/executable.elf: tmp/%.hwdef
	mkdir -p $(@D)
	$(HSI) -source scripts/fsbl.tcl -tclargs $* $(PROC)

tmp/%.tree/system.dts: tmp/%.hwdef $(DTREE_DIR)
	mkdir -p $(@D)
	$(HSI) -source scripts/devicetree.tcl -tclargs $* $(PROC) $(DTREE_DIR)
	patch $@ patches/devicetree.patch

clean:
	$(RM) uImage fw_printenv boot.bin devicetree.dtb tmp
	$(RM) .Xil usage_statistics_webtalk.html usage_statistics_webtalk.xml

