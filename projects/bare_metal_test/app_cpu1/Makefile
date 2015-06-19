TARGET    = app_cpu1.elf
OBJECTS   = app_cpu1.o
CFLAGS    = -MMD -MP
INCLUDES  = -Iapp_cpu1_bsp/ps7_cortexa9_1/include -I.
LDFLAGS   = -Wl,-T -Wl,lscript.ld -Lapp_cpu1_bsp/ps7_cortexa9_1/lib
LIBRARIES = -Wl,--start-group,-lxil,-lgcc,-lc,--end-group

CC        = arm-xilinx-eabi-gcc
RM        = rm -rf

$(TARGET): $(OBJECTS)
	@echo ">> Linking $@"
	@$(CC) $(LDFLAGS) $(LIBRARIES) -o $@ $^

%.o: %.c
	@echo ">> Compiling $<"
	@$(CC) -c $(CFLAGS) $(INCLUDES) -o $@ $<

clean:
	@$(RM) $(OBJECTS) $(TARGET)
