CFLAGS = -O3 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard -ffast-math -fsingle-precision-constant -mvectorize-with-neon-quad

all: measure-corr sleep-rand sleep-to-59 write-c2-files upload-to-pskreporter

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
	rm -rf measure-corr sleep-to-59 write-c2-files
