bits 32

; ─── Temporary page tables (identity map + higher half) ───────────────────────
; Each table is 4KB aligned, zeroed at boot
section .bss
align 4096
pml4:   resb 4096
pdpt:   resb 4096
pd:     resb 4096

; ─── Long mode entry point (64-bit) ───────────────────────────────────────────
section .text
extern  kmain
global  longmode_enter
global  longmode_64

; ─── longmode_enter ───────────────────────────────────────────────────────────
; Called from boot.asm (32-bit)
; ebx = Multiboot2 boot info address (preserved throughout)
longmode_enter:

    ; 1) Check CPU supports long mode
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb  .no_longmode

    mov eax, 0x80000001
    cpuid
    test edx, (1 << 29)     ; LM bit
    jz  .no_longmode

    ; 2) Build minimal page tables
    ;    We map the first 1GB twice:
    ;      identity map : 0x000000000 → 0x000000000  (needed during the switch)
    ;      higher half  : 0xFFFFFFFF80000000 → 0x000000000  (kernel lives here)

    ; PML4[0] → pdpt  (identity)
    mov eax, pdpt
    or  eax, 0x3            ; present + writable
    mov [pml4], eax

    ; PML4[511] → pdpt  (higher half — index 511 = 0xFFFF...80000000)
    mov [pml4 + 511 * 8], eax

    ; PDPT[0] → pd
    mov eax, pd
    or  eax, 0x3
    mov [pdpt], eax

    ; Map 512 × 2MB huge pages in PD → covers 0..1GB
    mov ecx, 0
.map_pd:
    mov eax, 0x200000       ; 2MB
    mul ecx                 ; eax = ecx * 2MB
    or  eax, 0x83           ; present + writable + huge page (PS bit)
    mov [pd + ecx * 8], eax
    inc ecx
    cmp ecx, 512
    jne .map_pd

    ; 3) Load PML4 into cr3
    mov eax, pml4
    mov cr3, eax

    ; 4) Enable PAE (bit 5 of cr4)
    mov eax, cr4
    or  eax, (1 << 5)
    mov cr4, eax

    ; 5) Set LME bit in EFER MSR (0xC0000080)
    mov ecx, 0xC0000080
    rdmsr
    or  eax, (1 << 8)       ; LME
    wrmsr

    ; 6) Enable paging (bit 31 of cr0) — triggers long mode
    mov eax, cr0
    or  eax, (1 << 31)
    mov cr0, eax

    ; 7) Far jump to 64-bit code segment (flushes instruction pipeline)
    ;    0x08 = GDT_CODE_SEG (kernel code, 64-bit, L=1)
    jmp 0x08:longmode_64

.no_longmode:
    hlt
    jmp .no_longmode

; ─── From here the CPU is in 64-bit long mode ─────────────────────────────────
bits 64

extern __bss_start
extern __bss_end
extern kmain

longmode_64:
    ; Reload data segment registers (null segment is fine in long mode)
    mov ax, 0x10            ; GDT_DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Save boot info address before we clobber registers
    mov edi, ebx            ; first argument to kmain (System V AMD64 ABI)

    ; Zero BSS — required before any C code runs
    ; memset(__bss_start, 0, __bss_end - __bss_start)
    mov rbx, __bss_start
    mov rcx, __bss_end
.zero_bss:
    cmp rbx, rcx
    jge .done_bss
    mov byte [rbx], 0
    inc rbx
    jmp .zero_bss
.done_bss:

    ; Call kmain(uint32_t multiboot_info_addr)
    call kmain

    ; kmain should never return — halt if it does
.hang:
    hlt
    jmp .hang