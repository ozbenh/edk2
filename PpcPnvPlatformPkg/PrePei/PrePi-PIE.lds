SECTIONS
{
  .text : ALIGN(0x10000) {
    __reloc_base = .;

    *(.text .text*)
    *(.rodata .rodata*)
  }

  .got : ALIGN(0x100) {
    *(.got)
    *(.toc)
  }
 
  .data : ALIGN(0x100) {
    *(.data .data*)

    __bss_start = .;
    *(.bss .bss* *COM*)
   . = ALIGN(0x10);
    __bss_end = .;

    __reloc_start = .;
    *(.rela .rela*)
    __reloc_end = .;
  }

  /DISCARD/ : {
    *(.note.GNU-stack)
    *(.gnu_debuglink)
    *(.interp)
    *(.dynamic)
    *(.dynsym)
    *(.dynstr)
    *(.hash)
    *(.comment)
  }
}
