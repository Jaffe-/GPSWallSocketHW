# Target name:
TARGET = fw

# Target type
MCU=atmega328p
PROGRAMMER=arduino
F_CPU=16000000L

MODULES = ./ $(shell ls -d */)
CSRC = $(foreach m, $(MODULES), $(wildcard $(m)*.c))
HEADERS = $(foreach m, $(MODULES), $(wildcard $(m)*.h))
OBJ = $(CSRC:.c=.o)

# Compiler / Linker flags:
CFLAGS = -mmcu=$(MCU) -Wall -O3 -std=gnu11 -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -I$(shell pwd) -DTARGET -DF_CPU=$(F_CPU)
LDFLAGS = -mmcu=$(MCU) -Wl,-Map=$(TARGET).map

###################################

.PHONY: compile flash clean

compile: $(TARGET).hex

flash: compile
	avrdude -c $(PROGRAMMER) -P $(PORT) -p $(MCU) -U flash:w:$(TARGET).hex

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
