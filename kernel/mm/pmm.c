#include <stdint.h>
#include <stddef.h>

void serial_puts(const char *s);

/* ─── Constants ─────────────────────────────────────────────────────────── */
#define PAGE_SIZE       4096
#define BITMAP_MAX_FRAMES (8 * 1024 * 1024)   /* supports up to 32GB RAM */

/* ─── Multiboot2 structures ─────────────────────────────────────────────── */
typedef struct {
    uint32_t total_size;
    uint32_t reserved;
} __attribute__((packed)) mb2_info_t;

typedef struct {
    uint32_t type;
    uint32_t size;
} __attribute__((packed)) mb2_tag_t;

typedef struct {
    uint32_t type;        /* = 6 */
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
} __attribute__((packed)) mb2_tag_mmap_t;

typedef struct {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;        /* 1 = available */
    uint32_t reserved;
} __attribute__((packed)) mb2_mmap_entry_t;

/* ─── Bitmap ────────────────────────────────────────────────────────────── */
static uint8_t  bitmap[BITMAP_MAX_FRAMES / 8];
static uint64_t total_frames = 0;
static uint64_t free_frames  = 0;

/* Kernel end address — set by linker script */
extern uint8_t __kernel_end;

/* ─── Bitmap helpers ────────────────────────────────────────────────────── */
static inline void bitmap_set(uint64_t frame) {
    bitmap[frame / 8] |= (1 << (frame % 8));
}

static inline void bitmap_clear(uint64_t frame) {
    bitmap[frame / 8] &= ~(1 << (frame % 8));
}

static inline int bitmap_test(uint64_t frame) {
    return bitmap[frame / 8] & (1 << (frame % 8));
}

/* ─── pmm_init ──────────────────────────────────────────────────────────── */
void pmm_init(uint64_t mb_info_addr) {
    uint64_t kernel_end = (uint64_t)&__kernel_end;

    for (uint64_t i = 0; i < BITMAP_MAX_FRAMES / 8; i++)
        bitmap[i] = 0xFF;

    mb2_tag_t *tag = (mb2_tag_t *)(mb_info_addr + 8);
    while (tag->type != 0) {
        if (tag->type == 6) {
            mb2_tag_mmap_t *mmap_tag = (mb2_tag_mmap_t *)tag;
            uint8_t *entry_ptr = (uint8_t *)tag + sizeof(mb2_tag_mmap_t);
            uint8_t *end_ptr   = (uint8_t *)tag + tag->size;

            while (entry_ptr < end_ptr) {
                mb2_mmap_entry_t *entry = (mb2_mmap_entry_t *)entry_ptr;

                if (entry->type == 1) {
                    uint64_t base = entry->base_addr;
                    uint64_t len  = entry->length;

                    uint64_t aligned_base = (base + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
                    if (aligned_base >= base + len) {
                        entry_ptr += mmap_tag->entry_size;
                        continue;
                    }
                    len -= (aligned_base - base);
                    base = aligned_base;

                    uint64_t frame_start = base / PAGE_SIZE;
                    uint64_t frame_count = len  / PAGE_SIZE;

                    for (uint64_t f = frame_start; f < frame_start + frame_count; f++) {
                        if (f >= BITMAP_MAX_FRAMES) break;
                        uint64_t frame_addr = f * PAGE_SIZE;
                        if (frame_addr < kernel_end) continue;
                        bitmap_clear(f);
                        free_frames++;
                        total_frames++;
                    }
                }
                entry_ptr += mmap_tag->entry_size;
            }
        }
        uint64_t next = (uint64_t)tag + tag->size;
        next = (next + 7) & ~7;
        tag = (mb2_tag_t *)next;
    }

    serial_puts("pmm: initialized\n");
}

/* ─── pmm_alloc ─────────────────────────────────────────────────────────── */
void *pmm_alloc(void) {
    for (uint64_t f = 0; f < BITMAP_MAX_FRAMES; f++) {
        if (!bitmap_test(f)) {
            bitmap_set(f);
            free_frames--;
            return (void *)(f * PAGE_SIZE);
        }
    }
    serial_puts("pmm: out of memory\n");
    return NULL;
}

uint64_t pmm_get_total_ram(void) {
    return total_frames * PAGE_SIZE;
}
void pmm_free(void *addr) {
    uint64_t frame = (uint64_t)addr / PAGE_SIZE;
    bitmap_clear(frame);
    free_frames++;
}