bits 32

; ─── GDT entries ──────────────────────────────────────────────────────────────
section .data
align 8
gdt_start:

; Null descriptor — required, all zeroes
gdt_null:
    dq 0

; Kernel code segment (ring 0, 64-bit)
; base=0, limit=0xFFFFF, G=1, L=1 (64-bit), P=1, DPL=0, S=1, type=exec/read
gdt_code:
    dw 0xFFFF       ; limit 15:0
    dw 0x0000       ; base 15:0
    db 0x00         ; base 23:16
    db 0x9A         ; P=1, DPL=00, S=1, type=1010 (exec/read)
    db 0xAF         ; G=1, D=0, L=1, limit 19:16 = 0xF
    db 0x00         ; base 31:24

; Kernel data segment (ring 0)
; base=0, limit=0xFFFFF, G=1, L=0, P=1, DPL=0, S=1, type=read/write
gdt_data:
    dw 0xFFFF       ; limit 15:0
    dw 0x0000       ; base 15:0
    db 0x00         ; base 23:16
    db 0x92         ; P=1, DPL=00, S=1, type=0010 (read/write)
    db 0xCF         ; G=1, D=1, L=0, limit 19:16 = 0xF
    db 0x00         ; base 31:24

gdt_end:

; ─── GDT descriptor (fed to lgdt) ─────────────────────────────────────────────
gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; size (bytes) - 1
    dd gdt_start                ; linear address of GDT

; ─── Segment selectors ────────────────────────────────────────────────────────
; selector = index * 8 (each entry is 8 bytes), RPL=00
GDT_CODE_SEG equ gdt_code - gdt_start   ; = 0x08
GDT_DATA_SEG equ gdt_data - gdt_start   ; = 0x10

; ─── gdt_load ─────────────────────────────────────────────────────────────────
section .text
global gdt_load
gdt_load:
    lgdt [gdt_descriptor]

    ; Reload data segments only — CS is left unchanged until longmode_enter
    ; (our GDT only has 64-bit code segment, reloading CS here would fault)
    mov ax, GDT_DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ret