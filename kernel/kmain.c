#include <stdint.h>

void serial_init(void);
void serial_puts(const char *s);
void idt_init(void);

void kmain(uint32_t multiboot_info_addr) {
    (void)multiboot_info_addr;

    serial_init();
    idt_init();
    serial_puts("kmain reached\n");

    /* Test IDT — trigger divide by zero */
    volatile int x = 1, y = 0;
    volatile int z = x / y;
    (void)z;

    for (;;)
        __asm__ volatile ("hlt");
}