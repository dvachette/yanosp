bits 64

extern exception_handler       ; defined in idt.c

; ─── Exception frame pushed on stack by CPU + our stubs ───────────────────────
; (top of stack when exception_handler is called)
;   [rsp+0]  = vector number
;   [rsp+8]  = error code (0 if none)
;   [rsp+16] = rip
;   [rsp+24] = cs
;   [rsp+32] = rflags
;   [rsp+40] = rsp (user)
;   [rsp+48] = ss  (user)

; ─── Macro: exception without error code ──────────────────────────────────────
%macro ISR_NOERR 1
global isr%1
isr%1:
    push qword 0        ; fake error code for uniform stack layout
    push qword %1       ; vector number
    jmp isr_common
%endmacro

; ─── Macro: exception with error code (CPU pushes it automatically) ───────────
%macro ISR_ERR 1
global isr%1
isr%1:
    push qword %1       ; vector number (error code already on stack)
    jmp isr_common
%endmacro

; ─── Exception stubs (vectors 0–31) ───────────────────────────────────────────
ISR_NOERR  0    ; #DE divide error
ISR_NOERR  1    ; #DB debug
ISR_NOERR  2    ; #NMI
ISR_NOERR  3    ; #BP breakpoint
ISR_NOERR  4    ; #OF overflow
ISR_NOERR  5    ; #BR bound range
ISR_NOERR  6    ; #UD invalid opcode
ISR_NOERR  7    ; #NM device not available
ISR_ERR    8    ; #DF double fault
ISR_NOERR  9    ; coprocessor segment overrun
ISR_ERR   10    ; #TS invalid TSS
ISR_ERR   11    ; #NP segment not present
ISR_ERR   12    ; #SS stack fault
ISR_ERR   13    ; #GP general protection fault
ISR_ERR   14    ; #PF page fault
ISR_NOERR 15    ; reserved
ISR_NOERR 16    ; #MF x87 FPU error
ISR_ERR   17    ; #AC alignment check
ISR_NOERR 18    ; #MC machine check
ISR_NOERR 19    ; #XF SIMD FP exception
ISR_NOERR 20    ; #VE virtualization exception
ISR_ERR   21    ; #CP control protection
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_ERR   30    ; #SX security exception
ISR_NOERR 31

; ─── Common handler stub ──────────────────────────────────────────────────────
isr_common:
    ; Save all general purpose registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; Pass pointer to stack frame as first argument (rdi)
    mov rdi, rsp
    call exception_handler

    ; Restore registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16     ; pop vector + error code
    iretq