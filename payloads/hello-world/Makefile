CROSS_COMPILE=arm-none-eabi-
CFLAGS=-Os -I../include -I../Lib/include -std=gnu99 -fomit-frame-pointer -nostartfiles -ffreestanding -Wextra -Werror

all : hello-world.elf hello-world.raw

clean:
	rm -f _start.o hello-world.elf hello-world.raw

%.o: %.S
	$(CROSS_COMPILE)gcc $(CFLAGS) -c -o $@ $^

hello-world.elf : linker.ld _start.o
	$(CROSS_COMPILE)ld -T $^ -o $@

hello-world.raw: hello-world.elf
	$(CROSS_COMPILE)objcopy -O binary $^ $@


