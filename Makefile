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

INITRAMFS_TAG = 3.18
LINUX_TAG = 6.1
DTREE_TAG = xilinx_v2023.1

INITRAMFS_DIR = tmp/initramfs-$(INITRAMFS_TAG)
LINUX_DIR = tmp/linux-$(LINUX_TAG)
DTREE_DIR = tmp/device-tree-xlnx-$(DTREE_TAG)

LINUX_TAR = tmp/linux-$(LINUX_TAG).tar.xz
DTREE_TAR = tmp/device-tree-xlnx-$(DTREE_TAG).tar.gz

INITRAMFS_URL = https://dl-cdn.alpinelinux.org/alpine/v$(INITRAMFS_TAG)/releases/armv7/netboot/initramfs-lts
LINUX_URL = https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-$(LINUX_TAG).55.tar.xz
DTREE_URL = https://github.com/Xilinx/device-tree-xlnx/archive/$(DTREE_TAG).tar.gz

SSBL_URL = https://github.com/pavel-demin/ssbl/releases/download/20231206/ssbl.elf

RTL8188_TAR = tmp/rtl8188eu-v5.2.2.4.tar.gz
RTL8188_URL = https://github.com/lwfinger/rtl8188eu/archive/v5.2.2.4.tar.gz

.PRECIOUS: tmp/cores/% tmp/%.xpr tmp/%.xsa tmp/%.bit tmp/%.fsbl/executable.elf tmp/%.tree/system-top.dts

all: tmp/$(NAME).bit boot.bin boot-rootfs.bin

cores: $(addprefix tmp/, $(CORES))

xpr: tmp/$(NAME).xpr

bit: tmp/$(NAME).bit

$(LINUX_TAR):
	mkdir -p $(@D)
	curl -L $(LINUX_URL) -o $@

$(DTREE_TAR):
	mkdir -p $(@D)
	curl -L $(DTREE_URL) -o $@

$(RTL8188_TAR):
	mkdir -p $(@D)
	curl -L $(RTL8188_URL) -o $@

$(INITRAMFS_DIR):
	mkdir -p $@
	curl -L $(INITRAMFS_URL) | gunzip | cpio -id --directory=$@
	patch -d $@ -p 0 < patches/initramfs.patch
	rm -rf $@/etc/modprobe.d $@/lib/firmware $@/lib/modules $@/var

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

tmp/ssbl.elf:
	curl -L $(SSBL_URL) -o $@

zImage.bin: $(LINUX_DIR)
	make -C $< mrproper
	make -C $< ARCH=arm -j $(shell nproc 2> /dev/null || echo 1) \
	  CROSS_COMPILE=arm-linux-gnueabihf- LOADADDR=0x8000 \
	  xilinx_zynq_defconfig zImage modules
	cp $</arch/arm/boot/zImage $@

initrd.bin: $(INITRAMFS_DIR)
	cd $< && find . | sort | cpio -o -H newc | gzip -9 -n > ../../$@
	truncate -s 4M $@

boot.bin: tmp/$(NAME).fsbl/executable.elf tmp/ssbl.elf initrd.dtb zImage.bin initrd.bin
	echo "img:{[bootloader] tmp/$(NAME).fsbl/executable.elf tmp/ssbl.elf [load=0x2000000] initrd.dtb [load=0x2008000] zImage.bin [load=0x3000000] initrd.bin}" > tmp/boot.bif
	bootgen -image tmp/boot.bif -w -o i $@

boot-rootfs.bin: tmp/$(NAME).fsbl/executable.elf tmp/ssbl.elf rootfs.dtb zImage.bin
	echo "img:{[bootloader] tmp/$(NAME).fsbl/executable.elf tmp/ssbl.elf [load=0x2000000] rootfs.dtb [load=0x2008000] zImage.bin}" > tmp/boot-rootfs.bif
	bootgen -image tmp/boot-rootfs.bif -w -o i $@

initrd.dtb: tmp/$(NAME).tree/system-top.dts
	dtc -I dts -O dtb -o $@ -i tmp/$(NAME).tree -i dts dts/initrd.dts

rootfs.dtb: tmp/$(NAME).tree/system-top.dts
	dtc -I dts -O dtb -o $@ -i tmp/$(NAME).tree -i dts dts/rootfs.dts

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

clean:
	$(RM) zImage.bin initrd.bin boot.bin boot-rootfs.bin initrd.dtb rootfs.dtb tmp
	$(RM) .Xil usage_statistics_webtalk.html usage_statistics_webtalk.xml
	$(RM) vivado*.jou vivado*.log
	$(RM) webtalk*.jou webtalk*.log
