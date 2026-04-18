#include <stdint.h>

void serial_puts(const char *s);

/* ─── IDT entry (16 bytes) ──────────────────────────────────────────────── */
typedef struct {
    uint16_t offset_low;    /* handler address bits 0-15  */
    uint16_t selector;      /* kernel code segment (0x08) */
    uint8_t  ist;           /* interrupt stack table (0 = none) */
    uint8_t  type_attr;     /* type + flags */
    uint16_t offset_mid;    /* handler address bits 16-31 */
    uint32_t offset_high;   /* handler address bits 32-63 */
    uint32_t zero;
} __attribute__((packed)) idt_entry_t;

/* ─── IDT descriptor (fed to lidt) ─────────────────────────────────────── */
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idt_descriptor_t;

#define IDT_SIZE 256
#define IDT_INT_GATE 0x8E   /* P=1, DPL=0, type=0xE (64-bit interrupt gate) */

static idt_entry_t idt[IDT_SIZE];
static idt_descriptor_t idt_desc;

/* ─── Exception names ───────────────────────────────────────────────────── */
static const char *exception_names[] = {
    "Division Error",           "Debug",
    "NMI",                      "Breakpoint",
    "Overflow",                 "Bound Range Exceeded",
    "Invalid Opcode",           "Device Not Available",
    "Double Fault",             "Coprocessor Segment Overrun",
    "Invalid TSS",              "Segment Not Present",
    "Stack Fault",              "General Protection Fault",
    "Page Fault",               "Reserved",
    "x87 FPU Error",            "Alignment Check",
    "Machine Check",            "SIMD FP Exception",
    "Virtualization Exception", "Control Protection",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Security Exception",       "Reserved"
};

/* ─── ISR forward declarations ──────────────────────────────────────────── */
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);
extern void isr3(void);  extern void isr4(void);  extern void isr5(void);
extern void isr6(void);  extern void isr7(void);  extern void isr8(void);
extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void isr16(void); extern void isr17(void);
extern void isr18(void); extern void isr19(void); extern void isr20(void);
extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void);
extern void isr27(void); extern void isr28(void); extern void isr29(void);
extern void isr30(void); extern void isr31(void);

static void (*isrs[])(void) = {
    isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7,
    isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15,
    isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
    isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
};

/* ─── idt_set_gate ──────────────────────────────────────────────────────── */
static void idt_set_gate(uint8_t vec, void (*handler)(void)) {
    uint64_t addr = (uint64_t)handler;
    idt[vec].offset_low  = addr & 0xFFFF;
    idt[vec].selector    = 0x08;
    idt[vec].ist         = 0;
    idt[vec].type_attr   = IDT_INT_GATE;
    idt[vec].offset_mid  = (addr >> 16) & 0xFFFF;
    idt[vec].offset_high = (addr >> 32) & 0xFFFFFFFF;
    idt[vec].zero        = 0;
}

/* ─── idt_init ──────────────────────────────────────────────────────────── */
void serial_puts(const char *s);

void idt_init(void) {
    idt_set_gate(0,  isrs[0]);  idt_set_gate(1,  isrs[1]);
    idt_set_gate(2,  isrs[2]);  idt_set_gate(3,  isrs[3]);
    idt_set_gate(4,  isrs[4]);  idt_set_gate(5,  isrs[5]);
    idt_set_gate(6,  isrs[6]);  idt_set_gate(7,  isrs[7]);
    idt_set_gate(8,  isrs[8]);  idt_set_gate(9,  isrs[9]);
    idt_set_gate(10, isrs[10]); idt_set_gate(11, isrs[11]);
    idt_set_gate(12, isrs[12]); idt_set_gate(13, isrs[13]);
    idt_set_gate(14, isrs[14]); idt_set_gate(15, isrs[15]);
    idt_set_gate(16, isrs[16]); idt_set_gate(17, isrs[17]);
    idt_set_gate(18, isrs[18]); idt_set_gate(19, isrs[19]);
    idt_set_gate(20, isrs[20]); idt_set_gate(21, isrs[21]);
    idt_set_gate(22, isrs[22]); idt_set_gate(23, isrs[23]);
    idt_set_gate(24, isrs[24]); idt_set_gate(25, isrs[25]);
    idt_set_gate(26, isrs[26]); idt_set_gate(27, isrs[27]);
    idt_set_gate(28, isrs[28]); idt_set_gate(29, isrs[29]);
    idt_set_gate(30, isrs[30]); idt_set_gate(31, isrs[31]);

    idt_desc.limit = sizeof(idt) - 1;
    idt_desc.base  = (uint64_t)idt;
    __asm__ volatile ("lidt %0" : : "m"(idt_desc));
}

/* ─── Stack frame layout (matches isr_common in idt.asm) ───────────────── */
typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t vector, error_code;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed)) exception_frame_t;

/* ─── exception_handler ─────────────────────────────────────────────────── */
void exception_handler(exception_frame_t *f) {
    serial_puts("\n=== EXCEPTION ===\n");
    serial_puts(exception_names[f->vector]);
    serial_puts("\n");
    for (;;) __asm__ volatile ("hlt");
}