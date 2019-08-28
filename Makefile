NAME = main
# Add any modules for which you want to use your own code for assign7, rest will be drawn from library
MY_MODULES = mcp3008.o spi.o pulsesensor.o display.o 

# This is the list of modules for building libmypi.a
LIMYBPI_MODULES = timer.o gpio.o strings.o printf.o backtrace.o malloc.o keyboard.o shell.o fb.o console.o

CFLAGS  = -I$(CS107E)/include -g -Wall -Wpointer-arith
CFLAGS += -Og -std=c99 -ffreestanding
CFLAGS += -mapcs-frame -fno-omit-frame-pointer -mpoke-function-name
LDFLAGS = -nostdlib -T memmap -L. -L$(CS107E)/lib
LDLIBS  = -lpi -lgcc

all : $(NAME).bin $(MY_MODULES)

%.bin: %.elf
	arm-none-eabi-objcopy $< -O binary $@

%.elf: %.o $(MY_MODULES) start.o cstart.o
	arm-none-eabi-gcc $(LDFLAGS) $^ $(LDLIBS) -o $@

%.o: %.c
	arm-none-eabi-gcc $(CFLAGS) -c $< -o $@

%.o: %.s
	arm-none-eabi-as $(ASFLAGS) $< -o $@

libmypi.a: $(LIMYBPI_MODULES) Makefile
	rm -f $@
	arm-none-eabi-ar cDr $@ $(filter %.o,$^)

%.list: %.o
	arm-none-eabi-objdump --no-show-raw-insn -d $< > $@

install: $(NAME).bin
	rpi-install.py -p $<

test: tests/test_keyboard_interrupts.bin
	rpi-install.py -p $<

bonus: $(NAME)-bonus.bin
	rpi-install.py -p $<

# Note: link is now against local libmypi first
%-bonus.elf: %.o start.o cstart.o libmypi.a
	arm-none-eabi-gcc $(LDFLAGS) $(filter %.o,$^) -lmypi $(LDLIBS) -o $@

clean:
	rm -f *.o *.bin *.elf *.list *~ libmypi.a

.PHONY: all clean install test bonus

.PRECIOUS: %.elf %.o %.a

# empty recipe used to disable built-in rules for native build
%:%.c
%:%.o

define CS107E_ERROR_MESSAGE
ERROR - CS107E environment variable is not set.

Please set it to point to the `cs107e.github.io/cs107e` directory using the
command `export CS107E=<replace with path to your cs107e.github.io directory>/cs107e`.

To have this done automatically, add the above command to your shell
environment configuration file (e.g. ~/.bashrc)
endef

ifndef CS107E
$(error $(CS107E_ERROR_MESSAGE))
endif
