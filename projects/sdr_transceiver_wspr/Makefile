OBJECTS = wsprsim_utils.o wsprd_utils.o tab.o fano.o nhash.o

vpath %.o wsprd

all: write-c2-files transmit-wspr-message

write-c2-files: write-c2-files.c
	gcc -o $@ $^ -lm -lconfig

transmit-wspr-message: transmit-wspr-message.c $(OBJECTS)
	gcc -Iwsprd -o $@ $^ -lm -lconfig
