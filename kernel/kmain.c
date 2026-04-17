#include <stdint.h>

void serial_init(void);
void serial_puts(const char *s);

void kmain(uint32_t multiboot_info_addr) {
    (void)multiboot_info_addr;  /* unused until phase 1.5 */

    serial_init();
    serial_puts("kmain reached, hello, Yanosp!\n");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}