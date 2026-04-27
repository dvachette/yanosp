# yanosp

![Architecture](https://img.shields.io/badge/arch-x86__64-blue?style=flat-square)
![Language](https://img.shields.io/badge/lang-C%20%7C%20ASM-informational?style=flat-square)
![Toolchain](https://img.shields.io/badge/toolchain-x86__64--elf--gcc-orange?style=flat-square)
![Bootloader](https://img.shields.io/badge/bootloader-GRUB2%20Multiboot2-red?style=flat-square)
![Status](https://img.shields.io/badge/status-in%20development-yellow?style=flat-square)
![License](https://img.shields.io/badge/license-MIT-green?style=flat-square)

A 64-bit monolithic UNIX-like kernel written from scratch in C and x86_64 assembly, with Linux syscall compatibility as the primary goal.

---

## Overview

**yanosp** is a hobby operating system targeting full Linux ABI compatibility — meaning existing Linux binaries compiled against glibc can run on it without modification. The kernel exposes the same syscall interface as Linux x86_64 (same numbers, same calling convention, same semantics), allowing glibc and userspace programs to function transparently.

The long-term goal is to run **Minecraft Java Edition**, which requires:
- A working JVM (OpenJDK port)
- OpenGL / Vulkan support via Mesa
- Intel HDA audio driver
- Full TCP/IP network stack
- A POSIX-compliant userspace

The project is built without GNU userland libraries (no glibc, no libstdc++ in the kernel itself). The kernel implements its own memory allocators, drivers, and subsystems from the ground up.

**Target hardware:** Intel i5 10th generation (x86_64, UHD 630 GPU)  
**Boot protocol:** GRUB2 + Multiboot2  
**Kernel model:** Monolithic  

---

## Repository structure

```
.
├── boot/               GRUB configuration
│   └── grub/
│       └── grub.cfg
├── kernel/
│   ├── arch/           x86_64-specific code (boot, GDT, IDT, TSS, long mode)
│   ├── drivers/        Hardware drivers (serial, ...)
│   ├── mm/             Memory management (PMM, VMM)
│   ├── proc/           Process management (ELF loader, scheduler, ...)
│   ├── fs/             VFS and filesystem drivers
│   ├── kmain.c         Kernel entry point
│   └── linker.ld       Kernel linker script
├── userspace/
│   └── init/           Minimal init process
├── build/              Compiled objects (generated)
├── isoroot/            ISO staging directory (generated)
├── Makefile
└── build-toolchain.sh  Cross-compiler build script
```

---

## Development environment setup

### Prerequisites

Install system dependencies:

```bash
sudo apt install build-essential bison flex libgmp-dev libmpc-dev libmpfr-dev \
                 texinfo nasm mtools xorriso qemu-system-x86 grub-pc-bin grub-common
```

### Cross-compiler (x86_64-elf-gcc)

The kernel requires a bare-metal cross-compiler targeting `x86_64-elf`. Build it from source using the provided script (~20-30 minutes):

```bash
chmod +x build-toolchain.sh
./build-toolchain.sh
```

This builds **binutils 2.42** and **GCC 14.1.0** and installs them to `/usr/local/cross`.

Add the toolchain to your PATH:

```bash
echo 'export PATH="/usr/local/cross/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

Verify:

```bash
x86_64-elf-gcc --version   # x86_64-elf-gcc (GCC) 14.1.0
nasm --version             # NASM version 2.x
qemu-system-x86_64 --version
grub-mkrescue --version
```

### Build and run

```bash
make          # build kernel + ISO
make run      # launch in QEMU
make clean    # remove build artifacts
```

QEMU is launched with `-serial stdio` — all kernel serial output appears in the terminal.

---

## Roadmap

### Phase 1 — Minimal bootable kernel
> **Status: complete ✅**

| Step | Description | Status |
|------|-------------|--------|
| 1.1 | GRUB2 Multiboot2 handoff, entry point, static stack | ✅ |
| 1.2 | GDT, long mode switch (32→64bit), minimal page tables | ✅ |
| 1.3 | BSS zeroing, `kmain()` entry in C | ✅ |
| 1.4 | Serial debug output (COM1, QEMU `-serial stdio`) | ✅ |
| 1.4.5 | Minimal IDT — CPU exception handlers (vectors 0–31) | ✅ |
| 1.5 | Physical memory manager — bitmap allocator, Multiboot2 memory map parsing | ✅ |
| 1.6 | Virtual memory manager — 4-level paging, `vmm_map`, physmap | ✅ |

---

### Phase 2 — Process execution
> **Status: in progress 🔄**

| Step | Description | Status |
|------|-------------|--------|
| 2.1 | ELF64 loader — PT_LOAD segments, embedded init binary | ✅ |
| 2.2 | Ring 0 / Ring 3 separation — TSS, user segments, `iretq` jump | 🔄 |
| 2.3 | IDT complete — PIC 8259 remap, IRQ handlers | 🔄 |
| 2.4 | Syscall interface — `syscall`/`sysret`, LSTAR/STAR/SFMASK MSRs | ⬜ |
| 2.5 | Process management — PCB, `fork`, `exec`, `exit`, `wait` | ⬜ |
| 2.6 | Scheduler — round-robin, timer interrupt (PIT/APIC) | ⬜ |

---

### Phase 3 — Filesystem
> **Status: planned ⬜**

| Step | Description |
|------|-------------|
| 3.1 | Block device driver — AHCI (QEMU), NVMe (physical hardware) |
| 3.2 | Block cache |
| 3.3 | VFS — inode/dentry abstraction layer |
| 3.4 | ramfs / tmpfs — in-memory filesystem for bootstrap |
| 3.5 | ext2/ext4 — on-disk filesystem |
| 3.6 | procfs / sysfs — `/proc`, `/sys` pseudo-filesystems |

---

### Phase 4 — Functional userspace
> **Status: planned ⬜**

| Step | Description |
|------|-------------|
| 4.1 | Dynamic linker — ELF `PT_INTERP`, `auxv` construction |
| 4.2 | glibc as shared library — copy from host distro |
| 4.3 | Minimal init system (PID 1) |
| 4.4 | Signal handling — `SIGKILL`, `SIGSEGV`, `SIGCHLD`, `rt_sigaction` |
| 4.5 | Shell + busybox (statically linked) |

---

### Phase 5 — Drivers
> **Status: planned ⬜**

| Step | Description |
|------|-------------|
| 5.1 | Keyboard / mouse — PS/2 (IRQ1), USB HID later |
| 5.2 | Framebuffer — VESA/GOP linear framebuffer via GRUB |
| 5.3 | GPU — DRM/KMS, Intel i915 (UHD 630) |
| 5.4 | Audio — Intel HDA controller + Realtek codec |
| 5.5 | Network — Intel e1000 (QEMU), Ethernet/ARP/IPv4/TCP/UDP, BSD sockets |

---

### Phase 6 — OpenGL / Vulkan / JVM
> **Status: planned ⬜**

| Step | Description |
|------|-------------|
| 6.1 | Mesa port — llvmpipe (software) then iris (Intel GPU) |
| 6.2 | Vulkan — ANV driver via Mesa |
| 6.3 | OpenJDK port — GC, JIT, `/proc/self/maps`, TLS |
| 6.4 | **Minecraft Java Edition** 🎯 |

---

## Technical notes

### Linux syscall compatibility

yanosp implements the Linux x86_64 syscall ABI:

```
rax = syscall number
rdi, rsi, rdx, r10, r8, r9 = arguments
rax = return value (negative errno on error)
```

glibc binaries compiled on any x86_64 Linux distribution will run without recompilation, as long as the required syscalls are implemented.

### Memory layout

```
0x0000000000000000 – 0x00007FFFFFFFFFFF   userspace (128 TB)
0xFFFF800000000000 – 0xFFFF8FFFFFFFFFFF   physmap (all physical RAM)
0xFFFFFFFF80000000 – 0xFFFFFFFFFFFFFFFF   kernel higher half
```

### Why not Wayland?

The physical machine runs KDE Plasma on X11. Wayland's limitations in low-level input control and window transparency are incompatible with the development workflow for this project.

---

## License

MIT — see [LICENSE](LICENSE).
