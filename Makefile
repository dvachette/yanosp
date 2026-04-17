# ─── Toolchain ────────────────────────────────────────────────────────────────
TARGET  := x86_64-elf
CC      := $(TARGET)-gcc
LD      := $(TARGET)-ld
ASM     := nasm

# ─── Flags ────────────────────────────────────────────────────────────────────
CFLAGS   := -std=c11 -ffreestanding -O2 -Wall -Wextra -nostdlib -nostdinc \
            -I/usr/local/cross/lib/gcc/x86_64-elf/14.1.0/include
LDFLAGS  := -T kernel/linker.ld -nostdlib
ASMFLAGS := -f elf64

# ─── Paths ────────────────────────────────────────────────────────────────────
KERNEL_DIR := kernel
BUILD_DIR  := build
ISO_DIR    := isoroot
BOOT_DIR   := $(ISO_DIR)/boot
GRUB_DIR   := $(BOOT_DIR)/grub

KERNEL_ELF := $(BOOT_DIR)/kernel.elf
ISO        := os.iso

# ─── Sources (auto-detected) ──────────────────────────────────────────────────
ASM_SRCS := $(wildcard $(KERNEL_DIR)/*.asm)
C_SRCS   := $(wildcard $(KERNEL_DIR)/*.c)

ASM_OBJS := $(patsubst $(KERNEL_DIR)/%.asm, $(BUILD_DIR)/%.o, $(ASM_SRCS))
C_OBJS   := $(patsubst $(KERNEL_DIR)/%.c,   $(BUILD_DIR)/%.o, $(C_SRCS))
OBJS     := $(ASM_OBJS) $(C_OBJS)

# ─── Targets ──────────────────────────────────────────────────────────────────
.PHONY: all run clean

all: $(ISO)

# Link kernel ELF
$(KERNEL_ELF): $(OBJS) | $(BOOT_DIR)
	$(LD) $(LDFLAGS) -o $@ $^

# Assemble .asm → build/*.o
$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.asm | $(BUILD_DIR)
	$(ASM) $(ASMFLAGS) $< -o $@

# Compile .c → build/*.o
$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Build ISO image
$(ISO): $(KERNEL_ELF) | $(GRUB_DIR)
	cp boot/grub/grub.cfg $(GRUB_DIR)/grub.cfg
	grub-mkrescue -o $(ISO) $(ISO_DIR) 2>/dev/null

# Create output dirs
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BOOT_DIR):
	mkdir -p $(BOOT_DIR)

$(GRUB_DIR):
	mkdir -p $(GRUB_DIR)

# Launch QEMU
run: $(ISO)
	qemu-system-x86_64  \
		-cdrom $(ISO)   \
		-boot d         \
		-serial stdio   \
		-m 256M         \
		-no-reboot      \
		-no-shutdown

clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR) $(ISO)