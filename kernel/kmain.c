#include <stdint.h>

void kmain(uint32_t multiboot_info_addr) {
    (void)multiboot_info_addr;  /* unused until phase 1.5 */

    /* Hang — serial output comes in 1.4 */
    for (;;) {
        __asm__ volatile ("hlt");
    }
}