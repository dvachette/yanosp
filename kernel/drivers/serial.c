#include <stdint.h>

/* COM1 base port and register offsets */
#define COM1            0x3F8
#define COM_DATA        (COM1 + 0)
#define COM_INT_ENABLE  (COM1 + 1)
#define COM_FIFO        (COM1 + 2)
#define COM_LINE_CTRL   (COM1 + 3)
#define COM_MODEM_CTRL  (COM1 + 4)
#define COM_LINE_STATUS (COM1 + 5)

/* Line status register bits */
#define LSR_TX_EMPTY    (1 << 5)    /* transmit buffer empty — ready to send */

/* ── Port I/O helpers ────────────────────────────────────────────────────── */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/* ── serial_init ─────────────────────────────────────────────────────────── */
void serial_init(void) {
    outb(COM_INT_ENABLE, 0x00);     /* disable interrupts                    */
    outb(COM_LINE_CTRL,  0x80);     /* enable DLAB (baud rate divisor mode)  */
    outb(COM_DATA,       0x03);     /* divisor low  byte → 38400 baud        */
    outb(COM_INT_ENABLE, 0x00);     /* divisor high byte                     */
    outb(COM_LINE_CTRL,  0x03);     /* 8 bits, no parity, 1 stop bit (8N1)   */
    outb(COM_FIFO,       0xC7);     /* enable FIFO, clear, 14-byte threshold */
    outb(COM_MODEM_CTRL, 0x0B);     /* RTS/DSR on                            */
}

/* ── serial_putc ─────────────────────────────────────────────────────────── */
void serial_putc(char c) {
    /* Poll until transmit buffer is empty */
    while (!(inb(COM_LINE_STATUS) & LSR_TX_EMPTY));
    outb(COM_DATA, (uint8_t)c);
}

/* ── serial_puts ─────────────────────────────────────────────────────────── */
void serial_puts(const char *s) {
    while (*s)
        serial_putc(*s++);
}