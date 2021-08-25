CROSS_COMPILE=arm-none-eabi-
CFLAGS=-Os -I../include -I../Lib/include -std=gnu99 -fomit-frame-pointer -nostartfiles -ffreestanding -Wextra -Werror

all : dump-sram.elf dump-sram.raw

clean:
	rm -f _start.o dump-sram.elf dump-sram.raw

%.o: %.S
	$(CROSS_COMPILE)gcc $(CFLAGS) -c -o $@ $^

dump-sram.elf : linker.ld _start.o
	$(CROSS_COMPILE)ld -T $^ -o $@

dump-sram.raw: dump-sram.elf
	$(CROSS_COMPILE)objcopy -O binary $^ $@


