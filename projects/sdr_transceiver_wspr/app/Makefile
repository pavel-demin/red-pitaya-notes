OBJECTS = wsprsim_utils.o wsprd_utils.o tab.o fano.o nhash.o

CFLAGS = -O3 -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard -ffast-math -fsingle-precision-constant -mvectorize-with-neon-quad

vpath %.o wsprd

all: gpio-output measure-corr sleep-to-59 write-c2-files transmit-wspr-message

gpio-output: gpio-output.c
	gcc $(CFLAGS) -o $@ $^

measure-corr: measure-corr.c
	gcc $(CFLAGS) -o $@ $^

sleep-to-59: sleep-to-59.c
	gcc $(CFLAGS) -o $@ $^

write-c2-files: write-c2-files.c
	gcc $(CFLAGS) -o $@ $^ -lm -lconfig

transmit-wspr-message: transmit-wspr-message.c $(OBJECTS)
	gcc $(CFLAGS) -Iwsprd -o $@ $^ -lm -lconfig

clean:
	rm -f gpio-output measure-corr write-c2-files transmit-wspr-message
