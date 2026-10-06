/* Linker script for STM32F429ZI: 2 MB flash, 192 KB SRAM (SRAM1+2+3). */
ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 192K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
  .isr_vector : { KEEP(*(.isr_vector)) } > FLASH

  .text :
  {
    *(.text*)
    *(.rodata*)
    . = ALIGN(4);
    _etext = .;
  } > FLASH

  .data : AT(_etext)
  {
    _sdata = .;
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > RAM

  .bss (NOLOAD) :
  {
    _sbss = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } > RAM
}
