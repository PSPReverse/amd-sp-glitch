CROSS_COMPILE=arm-none-eabi-
CFLAGS=-Os -I../include -I../Lib/include -std=gnu99 -fomit-frame-pointer -nostartfiles -ffreestanding -Wextra -Werror

all : get-secret-fuses.elf get-secret-fuses.raw

clean:
	rm -f _start.o get-secret-fuses.elf get-secret-fuses.raw

%.o: %.S
	$(CROSS_COMPILE)gcc $(CFLAGS) -c -o $@ $^

get-secret-fuses.elf : linker.ld _start.o
	$(CROSS_COMPILE)ld -T $^ -o $@

get-secret-fuses.raw: get-secret-fuses.elf
	$(CROSS_COMPILE)objcopy -O binary $^ $@


