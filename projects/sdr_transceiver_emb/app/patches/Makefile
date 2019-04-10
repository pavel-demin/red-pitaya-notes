TARGET   = libwdsp.a
SOURCES  = $(wildcard *.c)
OBJECTS  = $(SOURCES:.c=.o)
INCLUDES = -I.
CFLAGS   = -O3 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard -ffast-math -Wall
ARFLAGS  = cr
################################################################################
CC       = gcc
AR       = ar
RANLIB   = ranlib
RM       = rm -f
################################################################################
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo ">> Linking $@"
	@$(AR) $(ARFLAGS) $@ $^
	@$(RANLIB) $@

%.o: %.c
	@echo ">> Compiling $<"
	@$(CC) -c $(CFLAGS) $(INCLUDES) -o $@ $<

clean:
	@$(RM) $(OBJECTS)
	@$(RM) $(TARGET)
