CFLAGS = -O3 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard -ffast-math -fsingle-precision-constant -mvectorize-with-neon-quad

FT8D_TAG = master
FT8D_DIR = ft8d-$(FT8D_TAG)
FT8D_TAR = ft8d-$(FT8D_TAG).tar.gz
FT8D_URL = https://github.com/pavel-demin/ft8d/archive/$(FT8D_TAG).tar.gz

all: $(FT8D_DIR)/ft8d measure-corr sleep-rand sleep-to-59 write-c2-files upload-to-pskreporter

$(FT8D_TAR):
	curl -L $(FT8D_URL) -o $@

$(FT8D_DIR): $(FT8D_TAR)
	mkdir -p $@
	tar -zxf $< --strip-components=1 --directory=$@

$(FT8D_DIR)/ft8d: $(FT8D_DIR)
	make -C $<

measure-corr: measure-corr.c
	gcc $(CFLAGS) -o $@ $^

sleep-rand: sleep-rand.c
	gcc $(CFLAGS) -o $@ $^

sleep-to-59: sleep-to-59.c
	gcc $(CFLAGS) -o $@ $^

write-c2-files: write-c2-files.c
	gcc $(CFLAGS) -o $@ $^ -lm -lconfig

upload-to-pskreporter: upload-to-pskreporter.c
	gcc $(CFLAGS) -D_GNU_SOURCE -o $@ $^

clean:
	rm -rf $(FT8D_TAR) $(FT8D_DIR) measure-corr sleep-to-59 write-c2-files
