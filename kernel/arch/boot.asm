bits 32                         ; GRUB leaves us in protected 32-bits mode
extern gdt_load                 ; defined in gdt.asm
extern longmode_enter           ; defined in longmode.asm

; --- Multiboot2 constants -----------------------------------------------------
MB2_MAGIC    equ 0xE85250D6
MB2_ARCH     equ 0            ; i386
MB2_HDRLEN   equ (mb2_header_end - mb2_header)
MB2_CHECKSUM equ -(MB2_MAGIC + MB2_ARCH + MB2_HDRLEN)

; --- Multiboot2 header --------------------------------------------------------
; Must be within the first 32KB of the binary — placed first via linker script
section .multiboot2
align 8
mb2_header:
    dd MB2_MAGIC
    dd MB2_ARCH
    dd MB2_HDRLEN
    dd MB2_CHECKSUM
    ; End tag — required, signals end of tags
    dw 0                        ; type = 0
    dw 0                        ; flags = 0
    dd 8                        ; size = 8
mb2_header_end:

; --- Static stack -------------------------------------------------------------
section .bss
align 16
stack_bottom:
    resb 16384                  ; 16 KB
stack_top:

; --- Entry point --------------------------------------------------------------
section .text
global _start
_start:
    ; At this point:
    ;   eax = 0x36d76289  (Multiboot2 magic)
    ;   ebx = physical address of the boot info structure
    ;   protected mode 32-bit, interrupts disabled, no paging

    mov esp, stack_top          ; set up the stack

    ; ebx (boot info address) is preserved for kmain() later (phase 1.5)
    call gdt_load               ; load GDT and reload segment registers
    call longmode_enter         ; switch to 64-bit long mode (does not return)

.hang:
    hlt
    jmp .hang