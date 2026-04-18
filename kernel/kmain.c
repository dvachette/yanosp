#include <stdint.h>
void serial_init(void);
void serial_puts(const char *s);
void serial_putc(char c);
void idt_init(void);
void pmm_init(uint64_t mb_info_addr);
void *pmm_alloc(void);
void pmm_free(void *addr);

static void print_hex64(uint64_t v) {
    const char *h = "0123456789ABCDEF";
    char buf[19];
    buf[0] = '0'; buf[1] = 'x';
    for (int i = 0; i < 16; i++)
        buf[2 + i] = h[(v >> (60 - i * 4)) & 0xF];
    buf[18] = '\0';
    serial_puts(buf);
    serial_putc('\n');
}

void kmain(uint32_t multiboot_info_addr) {
    serial_init();
    idt_init();
    serial_puts("mb_info_addr = ");
    print_hex64((uint64_t)multiboot_info_addr);
    pmm_init((uint64_t)multiboot_info_addr);

    /* Test pmm_alloc */
    void *a = pmm_alloc();
    void *b = pmm_alloc();
    serial_puts("pmm_alloc a = "); print_hex64((uint64_t)a);
    serial_puts("pmm_alloc b = "); print_hex64((uint64_t)b);
    pmm_free(a);
    void *c = pmm_alloc();
    serial_puts("pmm_alloc c = "); print_hex64((uint64_t)c);

    serial_puts("kmain reached\n");

    for (;;)
        __asm__ volatile ("hlt");
}