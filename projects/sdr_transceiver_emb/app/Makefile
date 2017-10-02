CFLAGS = -O3 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard -ffast-math

WDSP_TAG = master
WDSP_DIR = wdsp-$(WDSP_TAG)
WDSP_TAR = wdsp-$(WDSP_TAG).tar.gz
WDSP_URL = https://github.com/g0orx/wdsp/archive/$(WDSP_TAG).tar.gz

all: sdr-transceiver-emb

$(WDSP_TAR):
	curl -L $(WDSP_URL) -o $@

$(WDSP_DIR): $(WDSP_TAR)
	mkdir -p $@
	tar -zxf $< --strip-components=1 --directory=$@
	rm -f $@/org_g0orx_openhpsdr_wdsp_WDSP.*
	patch -p 0 < patches/wdsp.patch
	cp patches/Makefile $@

$(WDSP_DIR)/libwdsp.a: $(WDSP_DIR)
	make -C $<

sdr-transceiver-emb: sdr-transceiver-emb.c $(WDSP_DIR)/libwdsp.a
	gcc $(CFLAGS) -o $@ $^  -I$(WDSP_DIR) -L$(WDSP_DIR) -lwdsp -lasound -lfftw3 -lm -lpthread

clean:
	rm -rf $(WDSP_TAR) $(WDSP_DIR) sdr-transceiver-emb
