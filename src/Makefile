# Target name:
TARGET = fw

# Target type
MCU=t85
PROGRAMMER=jtag3isp
F_CPU=8000000L

MODULES = ./ $(shell ls -d */)
CSRC = $(foreach m, $(MODULES), $(wildcard $(m)*.c))
HEADERS = $(foreach m, $(MODULES), $(wildcard $(m)*.h))
OBJ = $(CSRC:.c=.o)

# Compiler / Linker flags:
CFLAGS = -mmcu=$(MCU) -Wall -O3 -std=gnu11 -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -I$(shell pwd) -DTARGET
LDFLAGS = -mmcu=$(MCU) -Wl,-Map=$(TARGET).map

###################################

.PHONY: compile flash clean

compile: $(TARGET).hex

flash: compile
	avrdude -c $(PROGRAMMER) -P usb -p $(MCU) -B 2 -U flash:w:$(TARGET).hex

clean:
	rm -f $(OBJ) $(TARGET).{hex,map}

.SECONDARY: $(OBJS) 

###################################

%.hex: %.elf
	avr-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature $< $@

%.elf: $(OBJ)
	avr-gcc $^ $(LDFLAGS) -o $@

%.o : %.c
	avr-gcc $(CFLAGS) -c $< -o $@ 

-include $(CSRC:%.c=%.d)
