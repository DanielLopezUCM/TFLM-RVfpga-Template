
OUTPUT_ARCH( "riscv" )
ENTRY(_start)

SECTIONS {
  .text : { *(.text.init) *(.text*) }
  .rodata : { *(.rodata*) }
  .init_array : {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*)))
    KEEP (*(.init_array))
    PROVIDE_HIDDEN (__init_array_end = .);
  }
  . = ALIGN(16);
  .tflm_arena (NOLOAD) : {
    __tflm_arena_start = .;
    KEEP(*(.tflm_arena))
    __tflm_arena_end = .;
  }
  . = 0xd0580000;
  .data.io   : { *(.data.io) }
  . = 0xf0040000;
  __global_pointer$ = . + 0x800;
  .data : { *(.sdata*) *(.data*) }
  .bss (NOLOAD) : {
    . = ALIGN(4);
    __bss_start = .;
    *(.sbss*) *(.bss*) *(COMMON)
    . = ALIGN(4);
    __bss_end = .;
  }
  _end = .;
  STACK = ALIGN(16) + 0x1000;
  . = 0xfffffff8;
  .data.ctl : { LONG(0xf0040000); LONG(STACK) }
}
