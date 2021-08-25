
SECTIONS
{
  . = 0x100;
  .text ALIGN(0x10): { *(.text) }
  .data ALIGN(0x10) : { *(.data) }
  .bss ALIGN(0x10) : { *(.bss) }

}
