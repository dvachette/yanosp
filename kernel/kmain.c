#include <stdint.h>

void serial_init(void);
void serial_puts(const char *s);
void serial_putc(char c);
void idt_init(void);
void pmm_init(uint64_t mb_info_addr);
void *pmm_alloc(void);
void  pmm_free(void *addr);
uint64_t pmm_get_total_ram(void);
void vmm_init(uint64_t total_ram);

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
    vmm_init(pmm_get_total_ram());
    serial_puts("kmain reached\n");

    for (;;)
        __asm__ volatile ("hlt");
}