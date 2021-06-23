OBJECTS = wsprsim_utils.o wsprd_utils.o tab.o fano.o nhash.o

CFLAGS = -O3 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard -ffast-math -fsingle-precision-constant -mvectorize-with-neon-quad

vpath %.o ../wsprd

all: write-c2-files transmit-wspr-message

write-c2-files: write-c2-files.c
	gcc $(CFLAGS) -o $@ $^ -lm -lconfig

transmit-wspr-message: transmit-wspr-message.c $(OBJECTS)
	gcc $(CFLAGS) -I../wsprd -o $@ $^ -lm -lconfig

clean:
	rm -f write-c2-files transmit-wspr-message
