#pragma once

#include <common/common.h>

// Memory management-related Define
#define SATP_SV39 (8ULL << 60)
#define PAGE_V (1 << 0) // "Valid bit" (entry is enabled)
#define PAGE_R (1 << 1) // Readable
#define PAGE_W (1 << 2) // Writable
#define PAGE_X (1 << 3) // Executable
#define PAGE_U (1 << 4) // User (accessible in user mode)

// Process-related Define
#define PROCS_MAX 8
#define PROC_UNUSED 0
#define PROC_RUNNABLE 1
#define PROC_EXITED 2

// User space-related Define
// Base virtual address of an application image
// Check with user.ld
#define USER_BASE 0x1000000
// Switch to user mode
#define SSTATUS_SPIE (1 << 5)
#define SCAUSE_ECALL 8

// Virtio-related Define
#define SECTOR_SIZE       512
#define VIRTQ_ENTRY_NUM   16
#define VIRTIO_DEVICE_BLK 2
#define VIRTIO_BLK_PADDR  0x10001000

#define VIRTIO_REG_MAGIC         0x00
#define VIRTIO_REG_VERSION       0x04
#define VIRTIO_REG_DEVICE_ID     0x08
#define VIRTIO_REG_PAGE_SIZE     0x28
#define VIRTIO_REG_QUEUE_SEL     0x30
#define VIRTIO_REG_QUEUE_NUM_MAX 0x34
#define VIRTIO_REG_QUEUE_NUM     0x38
#define VIRTIO_REG_QUEUE_PFN     0x40
#define VIRTIO_REG_QUEUE_READY   0x44
#define VIRTIO_REG_QUEUE_NOTIFY  0x50
#define VIRTIO_REG_DEVICE_STATUS 0x70
#define VIRTIO_REG_DEVICE_CONFIG 0x100

#define VIRTIO_STATUS_ACK       1
#define VIRTIO_STATUS_DRIVER    2
#define VIRTIO_STATUS_DRIVER_OK 4

#define VIRTQ_DESC_F_NEXT          1
#define VIRTQ_DESC_F_WRITE         2
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1

#define VIRTIO_BLK_T_IN  0
#define VIRTIO_BLK_T_OUT 1

// Kernel Macros
#define PANIC(fmt, ...) \
	do {	\
		printf("%s PANIC: %s:%d: " fmt "\n", \
			KERNEL_NAME, __FILE__, __LINE__, ##__VA_ARGS__ \
		); \
		while (1) {} \
	} while (0) \

// Exception handling-macros
#define READ_CSR(reg) \
	({ \
		unsigned long __tmp; \
		__asm __volatile__("csrr %0, " #reg : "=r"(__tmp)); \
		__tmp; \
	})

#define WRITE_CSR(reg, value) \
	do { \
		uint64_t __tmp = (value); \
		__asm__ __volatile__("csrw " # reg ", %0" ::"r"(__tmp)); \
	} while(0)

// Exception handling-data structure
struct trap_frame {
	uint64_t ra;
	uint64_t gp;
	uint64_t tp;
	uint64_t t0;
	uint64_t t1;
	uint64_t t2;
	uint64_t t3;
	uint64_t t4;
	uint64_t t5;
	uint64_t t6;
	uint64_t a0;
	uint64_t a1;
	uint64_t a2;
	uint64_t a3;
	uint64_t a4;
	uint64_t a5;
	uint64_t a6;
	uint64_t a7;
	uint64_t s0;
	uint64_t s1;
	uint64_t s2;
	uint64_t s3;
	uint64_t s4;
	uint64_t s5;
	uint64_t s6;
	uint64_t s7;
	uint64_t s8;
	uint64_t s9;
	uint64_t s10;
	uint64_t s11;
	uint64_t sp;
} __attribute__((packed));

// Process-related structs
struct process {
	int pid;
	int state;
	vaddr_t sp;
	uint64_t *page_table;
	uint8_t stack[8192] __attribute__((aligned(16)));
};

// Virtio-related structs
struct virtq_desc {
	uint64_t addr;
	uint32_t len;
	uint16_t flags;
	uint16_t next;
} __attribute__((packed));

struct virtq_avail {
	uint16_t flags;
	uint16_t index;
	uint16_t ring[VIRTQ_ENTRY_NUM];
} __attribute__((packed));

struct virtq_used_elem {
	uint32_t id;
	uint32_t len;
} __attribute__((packed));

struct virtq_used {
	uint16_t flags;
	uint16_t index;
	struct virtq_used_elem ring[VIRTQ_ENTRY_NUM];
} __attribute__((packed));

struct virtio_virtq {
	struct virtq_desc descs[VIRTQ_ENTRY_NUM];
	struct virtq_avail avail;
	struct virtq_used used __attribute__((aligned(PAGE_SIZE)));

	int queue_index;
	volatile uint16_t *used_index; // 64-bit pointer in 64-bit mode
	uint16_t last_used_index;
};

struct virtio_blk_req {
	uint32_t type;
	uint32_t reserved; // Matches 64-bit alignment for 'sector'
	uint64_t sector;
	uint8_t data[SECTOR_SIZE];
	uint8_t status;
} __attribute__((packed));
