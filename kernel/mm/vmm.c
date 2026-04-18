#include <stdint.h>
#include <stddef.h>

void serial_puts(const char *s);
void *pmm_alloc(void);
void  pmm_free(void *addr);

/* ─── Page flags ─────────────────────────────────────────────────────────── */
#define PAGE_PRESENT    (1ULL << 0)
#define PAGE_WRITABLE   (1ULL << 1)
#define PAGE_USER       (1ULL << 2)
#define PAGE_HUGE       (1ULL << 7)
#define PAGE_NX         (1ULL << 63)

#define PAGE_KERNEL     (PAGE_PRESENT | PAGE_WRITABLE)
#define PAGE_KERNEL_RO  (PAGE_PRESENT)

/* ─── Address space constants ────────────────────────────────────────────── */
#define PAGE_SIZE       4096ULL
#define PHYSMAP_BASE    0xFFFF800000000000ULL   /* physical memory map */
#define KERNEL_BASE     0xFFFFFFFF80000000ULL   /* kernel higher half  */

/* ─── Index extraction from virtual address ──────────────────────────────── */
#define PML4_IDX(v)  (((v) >> 39) & 0x1FF)
#define PDPT_IDX(v)  (((v) >> 30) & 0x1FF)
#define PD_IDX(v)    (((v) >> 21) & 0x1FF)
#define PT_IDX(v)    (((v) >> 12) & 0x1FF)

/* ─── Entry helpers ──────────────────────────────────────────────────────── */
#define ENTRY_ADDR(e)    ((e) & 0x000FFFFFFFFFF000ULL)
#define ENTRY_PRESENT(e) ((e) & PAGE_PRESENT)

/* ─── Kernel symbols ─────────────────────────────────────────────────────── */
extern uint8_t __kernel_end;

/* ─── Current PML4 (physical address) ───────────────────────────────────── */
static uint64_t *kernel_pml4 = NULL;

/* ─── Allocate a zeroed page for a table ────────────────────────────────── */
static uint64_t *alloc_table(void) {
    uint64_t *t = (uint64_t *)pmm_alloc();
    if (!t) return NULL;
    for (int i = 0; i < 512; i++) t[i] = 0;
    return t;
}

/* ─── vmm_map ────────────────────────────────────────────────────────────── */
/* Maps virtual address `virt` to physical address `phys` with given flags.  */
/* All addresses must be 4KB aligned.                                        */
void vmm_map(uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t *pml4 = kernel_pml4;

    /* PML4 → PDPT */
    uint64_t *pdpt;
    if (!ENTRY_PRESENT(pml4[PML4_IDX(virt)])) {
        pdpt = alloc_table();
        pml4[PML4_IDX(virt)] = (uint64_t)pdpt | PAGE_KERNEL;
    } else {
        pdpt = (uint64_t *)ENTRY_ADDR(pml4[PML4_IDX(virt)]);
    }

    /* PDPT → PD */
    uint64_t *pd;
    if (!ENTRY_PRESENT(pdpt[PDPT_IDX(virt)])) {
        pd = alloc_table();
        pdpt[PDPT_IDX(virt)] = (uint64_t)pd | PAGE_KERNEL;
    } else {
        pd = (uint64_t *)ENTRY_ADDR(pdpt[PDPT_IDX(virt)]);
    }

    /* PD → PT */
    uint64_t *pt;
    if (!ENTRY_PRESENT(pd[PD_IDX(virt)])) {
        pt = alloc_table();
        pd[PD_IDX(virt)] = (uint64_t)pt | PAGE_KERNEL;
    } else {
        pt = (uint64_t *)ENTRY_ADDR(pd[PD_IDX(virt)]);
    }

    /* PT → physical frame */
    pt[PT_IDX(virt)] = phys | flags;
}

/* ─── vmm_unmap ──────────────────────────────────────────────────────────── */
void vmm_unmap(uint64_t virt) {
    uint64_t *pml4 = kernel_pml4;
    if (!ENTRY_PRESENT(pml4[PML4_IDX(virt)])) return;
    uint64_t *pdpt = (uint64_t *)ENTRY_ADDR(pml4[PML4_IDX(virt)]);
    if (!ENTRY_PRESENT(pdpt[PDPT_IDX(virt)])) return;
    uint64_t *pd = (uint64_t *)ENTRY_ADDR(pdpt[PDPT_IDX(virt)]);
    if (!ENTRY_PRESENT(pd[PD_IDX(virt)])) return;
    uint64_t *pt = (uint64_t *)ENTRY_ADDR(pd[PD_IDX(virt)]);

    pt[PT_IDX(virt)] = 0;
    __asm__ volatile ("invlpg (%0)" : : "r"(virt) : "memory");
}

/* ─── vmm_init ───────────────────────────────────────────────────────────── */
void vmm_init(uint64_t total_ram) {
    /* Allocate new PML4 */
    kernel_pml4 = alloc_table();

    /* Identity map first 1GB (keep working during transition) */
    for (uint64_t phys = 0; phys < 0x40000000ULL; phys += PAGE_SIZE)
        vmm_map(phys, phys, PAGE_KERNEL);

    /* Physmap — map all physical RAM at PHYSMAP_BASE */
    for (uint64_t phys = 0; phys < total_ram; phys += PAGE_SIZE)
        vmm_map(PHYSMAP_BASE + phys, phys, PAGE_KERNEL);

    /* Map kernel at KERNEL_BASE */
    uint64_t kernel_size = (uint64_t)&__kernel_end - 0x100000ULL;
    for (uint64_t off = 0; off < kernel_size; off += PAGE_SIZE)
        vmm_map(KERNEL_BASE + off, 0x100000ULL + off, PAGE_KERNEL);

    /* Switch to new page tables */
    __asm__ volatile ("mov %0, %%cr3" : : "r"((uint64_t)kernel_pml4) : "memory");

    serial_puts("vmm: initialized\n");
}

/* ─── vmm_get_pml4 ───────────────────────────────────────────────────────── */
uint64_t vmm_get_pml4(void) {
    return (uint64_t)kernel_pml4;
}