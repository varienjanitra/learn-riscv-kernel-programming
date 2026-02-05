ENTRY(boot)

SECTIONS {
    /* 1. Start address for RV64 Supervisor mode */
    . = 0x80200000;
    __kernel_base = .;

    /* 2. Code Section */
    .text : ALIGN(4096) {
        KEEP(*(.text.boot));
        *(.text .text.*);
    }

    .rodata : ALIGN(4096) {
        *(.rodata .rodata.*);
    }

    .data : ALIGN(4096) {
        *(.data .data.*);
    }

    /* 4. BSS (Zero-initialized data) */
    .bss : ALIGN(4096) {
        __bss = .;
        *(.bss .bss.* .sbss .sbss.*);
        __bss_end = .;
    }

    /* 5. Stack Setup */
    . = ALIGN(4096);
    . += 128 * 1024; /* 128KB */
    __stack_top = .;

    /* 6. Free RAM (heap) Setup */
    . = ALIGN(4096);
    __free_ram = .;
    . += 64 * 1024 * 1024; /* 64 MB */
    __free_ram_end = .;
}