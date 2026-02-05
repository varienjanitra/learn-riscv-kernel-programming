# Compiler and Toolchain
CC = clang
OBJCOPY = llvm-objcopy
QEMU = qemu-system-riscv64

# Flags
CFLAGS = -std=c23 -O2 -g3 -Wall -Wextra --target=riscv64-unknown-elf \
         -mcmodel=medany -fno-PIC -fno-stack-protector -ffreestanding \
         -nostdlib -Iinclude

LDFLAGS = -fuse-ld=lld

# Directories
SRC_DIR = src
DIST_DIR = dist
SCRIPTS_DIR = scripts

# Final Artifacts
KERNEL_ELF = $(DIST_DIR)/kernel.elf
SHELL_BIN_O = $(DIST_DIR)/shell.bin.o

# --- Build Rules ---

.PHONY: all clean run

all: $(KERNEL_ELF)

# 1. Build User Shell
$(DIST_DIR)/shell.elf: $(SRC_DIR)/user/shell.c $(SRC_DIR)/user/user.c $(SRC_DIR)/common/common.c
	@mkdir -p $(DIST_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -Wl,-T$(SCRIPTS_DIR)/user.ld -Wl,-Map=$(DIST_DIR)/shell.map -o $@ $^

$(DIST_DIR)/shell.bin: $(DIST_DIR)/shell.elf
	$(OBJCOPY) --set-section-flags .bss=alloc,contents -O binary $< $@

# The "Pro Move": CD into dist to keep symbol names clean!
$(SHELL_BIN_O): $(DIST_DIR)/shell.bin
	(cd $(DIST_DIR) && $(OBJCOPY) -Ibinary -Oelf64-littleriscv -B riscv64:rv64 shell.bin shell.bin.o)

# 2. Build Kernel
$(KERNEL_ELF): $(SRC_DIR)/kernel/kernel.c $(SRC_DIR)/common/common.c $(SHELL_BIN_O)
	@mkdir -p $(DIST_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -Wl,-T$(SCRIPTS_DIR)/kernel.ld -Wl,-Map=$(DIST_DIR)/kernel.map -o $@ $^

# --- Utils ---

run: all
	$(QEMU) -machine virt -bios default -nographic -serial mon:stdio --no-reboot \
	    -d unimp,guest_errors,int,cpu_reset -D $(DIST_DIR)/qemu.log \
	    -drive id=drive0,file=lorem.txt,format=raw,if=none \
	    -device virtio-blk-device,drive=drive0,bus=virtio-mmio-bus.0 \
	    -kernel $(KERNEL_ELF)

clean:
	rm -rf $(DIST_DIR)