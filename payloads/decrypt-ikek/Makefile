CROSS_COMPILE=arm-none-eabi-
CFLAGS=-Os -I../include -I../Lib/include -std=gnu99 -fomit-frame-pointer -nostartfiles -ffreestanding -Wextra -Werror

all : decrypt-ikek.elf decrypt-ikek.raw

clean:
	rm -f _start.o decrypt-ikek.elf decrypt-ikek.raw

%.o: %.S
	$(CROSS_COMPILE)gcc $(CFLAGS) -c -o $@ $^

decrypt-ikek.elf : linker.ld _start.o
	$(CROSS_COMPILE)ld -T $^ -o $@

decrypt-ikek.raw: decrypt-ikek.elf
	$(CROSS_COMPILE)objcopy -O binary $^ $@


