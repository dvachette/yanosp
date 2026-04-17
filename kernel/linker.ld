OUTPUT_FORMAT(elf32-i386)
ENTRY(_start)

SECTIONS
{
    . = 0x100000;               /* load kernel at 1MB */

    /* Multiboot2 header must be first — GRUB scans the first 32KB */
    .multiboot2 : {
        *(.multiboot2)
    }

    /* Executable code */
    .text : {
        *(.text)
    }

    /* Read-only data (string literals, const globals) */
    .rodata : {
        *(.rodata)
    }

    /* Initialized global variables */
    .data : {
        *(.data)
    }

    /* Uninitialized globals — zeroed by boot.asm before kmain() */
    .bss : {
        __bss_start = .;        /* symbol exposed to boot.asm */
        *(.bss)
        *(COMMON)               /* uninitialized C globals (old GCC behavior) */
        __bss_end = .;          /* symbol exposed to boot.asm */
    }

    __kernel_end = .;           /* physical end of the kernel — used by PMM in 1.5 */
}