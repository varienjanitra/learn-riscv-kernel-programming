#include <common/common.h>
#include <kernel/kernel.h>

extern char __bss[], __bss_end[], __stack_top[];
extern char __free_ram[], __free_ram_end[];
extern char __kernel_base[];

extern char _binary_shell_bin_start[];
extern char _binary_shell_bin_end[];

struct process procs[PROCS_MAX];

struct process *current_proc;
struct process *idle_proc;

struct virtio_virtq *blk_request_vq;
struct virtio_blk_req *blk_req;
paddr_t blk_req_paddr;
uint64_t blk_capacity;

paddr_t alloc_pages(uint64_t n)
{
	static paddr_t next_paddr = (paddr_t) __free_ram;
	paddr_t paddr = next_paddr;

	next_paddr += n * PAGE_SIZE;

	if (next_paddr > (paddr_t) __free_ram_end)
		PANIC("Out of memory");

	memset((void *) paddr, 0, n * PAGE_SIZE);

	return paddr;
}

void map_page(uint64_t *table2, uint64_t vaddr, paddr_t paddr, uint64_t flags)
{
	if(!is_aligned(vaddr, PAGE_SIZE))
		PANIC("Unaligned vaddr %p", vaddr);

	if(!is_aligned(paddr, PAGE_SIZE))
		PANIC("Unaligned paddr %p", paddr);

	// Level 2 (Top Level)
	uint64_t vpn2 = (vaddr >> 30) & 0x1ff;
	if ((table2[vpn2] & PAGE_V) == 0) {
		uint64_t pt_paddr = alloc_pages(1);
		table2[vpn2] = ((pt_paddr / PAGE_SIZE) << 10 | PAGE_V);
	}

	// Level 1 (Middle Level)
	uint64_t *table1 = (uint64_t *) ((table2[vpn2] >> 10) * PAGE_SIZE);
	uint64_t vpn1 = (vaddr >> 21) & 0x1ff;
	if ((table1[vpn1] & PAGE_V) == 0) {
		uint64_t pt_paddr = alloc_pages(1);
		table1[vpn1] = ((pt_paddr / PAGE_SIZE) << 10 | PAGE_V);
	}

	// Level 0 (Leaf Level)
	uint64_t *table0 = (uint64_t *) ((table1[vpn1] >> 10) * PAGE_SIZE);
	uint64_t vpn0 = (vaddr >> 12) & 0x1ff;

	// Map the actual physical address
	table0[vpn0] = ((paddr / PAGE_SIZE) << 10) | flags | PAGE_V;
}

__attribute__((naked))
__attribute__((aligned(8)))
void kernel_entry(void)
{
	__asm__ __volatile__(
		// Retrieve the kernel stack of the running process from sscratch
		"csrrw sp, sscratch, sp\n"

		// Allocate space for 31 registers (8 bytes each)
		// Extra 1 space (32 registers) for 16-byte memory alignment purpose
		// Allocating 256 byte to keep the stack 16-byte aligned
		"addi sp, sp, -8 * 32\n"

		// Save the registers
		"sd ra, 8 * 0(sp)\n"
		"sd gp, 8 * 1(sp)\n"
		"sd tp, 8 * 2(sp)\n"
		"sd t0, 8 * 3(sp)\n"
		"sd t1, 8 * 4(sp)\n"
		"sd t2, 8 * 5(sp)\n"
		"sd t3, 8 * 6(sp)\n"
		"sd t4, 8 * 7(sp)\n"
		"sd t5, 8 * 8(sp)\n"
		"sd t6, 8 * 9(sp)\n"
		"sd a0, 8 * 10(sp)\n"
		"sd a1, 8 * 11(sp)\n"
		"sd a2, 8 * 12(sp)\n"
		"sd a3, 8 * 13(sp)\n"
		"sd a4, 8 * 14(sp)\n"
		"sd a5, 8 * 15(sp)\n"
		"sd a6, 8 * 16(sp)\n"
		"sd a7, 8 * 17(sp)\n"
		"sd s0, 8 * 18(sp)\n"
		"sd s1, 8 * 19(sp)\n"
		"sd s2, 8 * 20(sp)\n"
		"sd s3, 8 * 21(sp)\n"
		"sd s4, 8 * 22(sp)\n"
		"sd s5, 8 * 23(sp)\n"
		"sd s6, 8 * 24(sp)\n"
		"sd s7, 8 * 25(sp)\n"
		"sd s8, 8 * 26(sp)\n"
		"sd s9, 8 * 27(sp)\n"
		"sd s10, 8 * 28(sp)\n"
		"sd s11, 8 * 29(sp)\n"

		// Grab the original SP from sscratch and save it
		"csrr a0, sscratch\n"
		"sd a0, 8 * 30(sp)\n"

		// Reset the kernel stack
		"addi a0, sp, 8 * 31\n"
		"csrw sscratch, a0\n"

		// Pass the pointer to the saved registers to handle trap
		"mv a0, sp\n"
		"call handle_trap\n"

		// Restore the registers
		"ld ra, 8 * 0(sp)\n"
		"ld gp, 8 * 1(sp)\n"
		"ld tp, 8 * 2(sp)\n"
		"ld t0, 8 * 3(sp)\n"
		"ld t1, 8 * 4(sp)\n"
		"ld t2, 8 * 5(sp)\n"
		"ld t3, 8 * 6(sp)\n"
		"ld t4, 8 * 7(sp)\n"
		"ld t5, 8 * 8(sp)\n"
		"ld t6, 8 * 9(sp)\n"
		"ld a0, 8 * 10(sp)\n"
		"ld a1, 8 * 11(sp)\n"
		"ld a2, 8 * 12(sp)\n"
		"ld a3, 8 * 13(sp)\n"
		"ld a4, 8 * 14(sp)\n"
		"ld a5, 8 * 15(sp)\n"
		"ld a6, 8 * 16(sp)\n"
		"ld a7, 8 * 17(sp)\n"
		"ld s0, 8 * 18(sp)\n"
		"ld s1, 8 * 19(sp)\n"
		"ld s2, 8 * 20(sp)\n"
		"ld s3, 8 * 21(sp)\n"
		"ld s4, 8 * 22(sp)\n"
		"ld s5, 8 * 23(sp)\n"
		"ld s6, 8 * 24(sp)\n"
		"ld s7, 8 * 25(sp)\n"
		"ld s8, 8 * 26(sp)\n"
		"ld s9, 8 * 27(sp)\n"
		"ld s10, 8 * 28(sp)\n"
		"ld s11, 8 * 29(sp)\n"
		"ld sp, 8 * 30(sp)\n"
		"sret\n"
	);
}

__attribute__((naked))
void user_entry(void)
{
	__asm__ __volatile__(
		"csrw sepc, %[sepc]\n"
		"csrw sstatus, %[sstatus]\n"
		"li sp, 0x1012000\n"
		"sret\n"
		:
		: [sepc] "r" (USER_BASE), \
		  [sstatus] "r" (SSTATUS_SPIE)
	);
}

__attribute__((naked))
void switch_context(uint64_t *prev_sp, uint64_t *next_sp)
{
	__asm__ __volatile__ (
		// Allocate space for 13 registers (8 bytes each)
		// Extra 1 space (14 registers) for 16-byte memory alignment purpose
		// Allocating 112 byte to keep the stack 16-byte aligned
		"addi sp, sp, -13 * 8 - 8\n"

		// Save the callee-saved registers
		"sd ra, 0 * 8(sp)\n"
		"sd s0, 1 * 8(sp)\n"
		"sd s1, 2 * 8(sp)\n"
		"sd s2, 3 * 8(sp)\n"
		"sd s3, 4 * 8(sp)\n"
		"sd s4, 5 * 8(sp)\n"
		"sd s5, 6 * 8(sp)\n"
		"sd s6, 7 * 8(sp)\n"
		"sd s7, 8 * 8(sp)\n"
		"sd s8, 9 * 8(sp)\n"
		"sd s9, 10 * 8(sp)\n"
		"sd s10, 11 * 8(sp)\n"
		"sd s11, 12 * 8(sp)\n"

		// Switch the stack pointer
		"sd sp, (a0)\n"
		"ld sp, (a1)\n"

		// Restore callee-saved registers
		"ld ra, 0 * 8(sp)\n"
		"ld s0, 1 * 8(sp)\n"
		"ld s1, 2 * 8(sp)\n"
		"ld s2, 3 * 8(sp)\n"
		"ld s3, 4 * 8(sp)\n"
		"ld s4, 5 * 8(sp)\n"
		"ld s5, 6 * 8(sp)\n"
		"ld s6, 7 * 8(sp)\n"
		"ld s7, 8 * 8(sp)\n"
		"ld s8, 9 * 8(sp)\n"
		"ld s9, 10 * 8(sp)\n"
		"ld s10, 11 * 8(sp)\n"
		"ld s11, 12 * 8(sp)\n"

		// Deallocate and return
		"addi sp, sp, 13 * 8 + 8\n"
		"ret\n"
	);
}

struct process *create_process(const void *image, size_t image_size)
{
	struct process *proc = nullptr;

	int i;
	for (i = 0; i < PROCS_MAX; i++) {
		if (procs[i].state == PROC_UNUSED) {
			proc = &procs[i];
			break;
		}
	}

	if (!proc)
		PANIC("No free process slots");

	uint64_t *sp = (uint64_t *) &proc->stack[sizeof(proc->stack)];

	*--sp = 0; // Slot 14 (extra)
	*--sp = 0; // s11
	*--sp = 0; // s10
	*--sp = 0; // s9
	*--sp = 0; // s8
	*--sp = 0; // s7
	*--sp = 0; // s6
	*--sp = 0; // s5
	*--sp = 0; // s4
	*--sp = 0; // s3
	*--sp = 0; // s2
	*--sp = 0; // s1
	*--sp = 0; // s0
	*--sp = (uint64_t) user_entry; // ra

	// Map kernel pages
	uint64_t *page_table = (uint64_t *) alloc_pages(1);
	for (paddr_t paddr = (paddr_t) __kernel_base;
			paddr < (paddr_t) __free_ram_end;
			paddr += PAGE_SIZE)
		map_page(page_table,
			paddr, paddr,
			PAGE_R | PAGE_W | PAGE_X);

	// Map user pages
	for (uint64_t off = 0; off < image_size; off += PAGE_SIZE) {
		paddr_t page = alloc_pages(1);

		size_t remaining = image_size - off;
		size_t copy_size = PAGE_SIZE <= remaining ? PAGE_SIZE : remaining;

		memcpy((void *) page, image + off, copy_size);
		map_page(page_table, USER_BASE + off, page, PAGE_U | PAGE_R | PAGE_W | PAGE_X);
	}

	// Map Virtio pages
	for (paddr_t paddr = (paddr_t) __kernel_base;
			paddr < (paddr_t) __free_ram_end;
			paddr += PAGE_SIZE)
		map_page(page_table, VIRTIO_BLK_PADDR, VIRTIO_BLK_PADDR, PAGE_R | PAGE_W);

	paddr_t stack_page = alloc_pages(1);
	map_page(page_table, 0x1011000, stack_page, PAGE_U | PAGE_R | PAGE_W);

	proc->pid = i + 1;
	proc->state = PROC_RUNNABLE;
	proc->sp = (uint64_t) sp;
	proc->page_table = page_table;

	return proc;
}

void delay(int time)
{
	for (int i = 0; i < time; i++)
		__asm__ __volatile__("nop");
}

void yield(void)
{
	struct process *next = idle_proc;

	for (int i = 0; i < PROCS_MAX; i++) {
		struct process *proc = &procs[(current_proc->pid + i) % PROCS_MAX];

		if(proc->state == PROC_RUNNABLE && proc->pid > 0) {
			next = proc;
			break;
		}
	}

	if (next == current_proc)
		return;

	__asm__ __volatile__(
                "sfence.vma\n"
                "csrw satp, %[satp]\n"
                "sfence.vma\n"
                "csrw sscratch, %[sscratch]\n"
                :
                : [satp] "r" (SATP_SV39 | ((uint64_t) next->page_table / PAGE_SIZE)),
                  [sscratch] "r" ((uint64_t) &next->stack[sizeof(next->stack)])
        );

	struct process *prev = current_proc;
	current_proc = next;
	switch_context(&prev->sp, &next->sp);
}

void handle_syscall(struct trap_frame *f)
{
	switch (f->a3) {
	case SYS_PUTCHAR:
		putchar(f->a0);
		break;
	case SYS_GETCHAR:
		while (1) {
			long ch = getchar();
			if (ch >= 0) {
				f->a0 = ch;
				break;
			}

			yield();
		}
		break;
	case SYS_EXIT:
		printf("Process %d exited\n", current_proc->pid);
		current_proc->state = PROC_EXITED;
		yield();
		PANIC("Unreachable point from SYS_EXIT");
	default:
		PANIC("Unexpected syscall a3 = %p\n", f->a3);
	}
}

void handle_trap(struct trap_frame *f)
{
	uint64_t scause = READ_CSR(scause);
	uint64_t stval = READ_CSR(stval);
	uint64_t user_pc = READ_CSR(sepc);

	if (scause == SCAUSE_ECALL) {
		handle_syscall(f);
		user_pc += 4;
	} else {
		PANIC("Unexpected trap:\n" \
			"scause = %p\n" \
			"stval = %p\n" \
			"sepc = %p\n" \
			, scause, stval, user_pc);
	}

	WRITE_CSR(sepc, user_pc);
}

// Virtio-related helper functions
static inline uintptr_t virtio_reg_addr(unsigned offset)
{
	return (uintptr_t)VIRTIO_BLK_PADDR + offset;
}

uint32_t virtio_reg_read32(unsigned offset)
{
	return *((volatile uint32_t *) virtio_reg_addr(offset));
}

void virtio_reg_write32(unsigned offset, uint32_t value)
{
	*((volatile uint32_t *) virtio_reg_addr(offset)) = value;
	__asm__ __volatile__("fence o, io" : : : "memory");
}

uint64_t virtio_reg_read64(unsigned offset)
{
	return *((volatile uint64_t *) virtio_reg_addr(offset));
}

void virtio_reg_fetch_and_or32(unsigned offset, uint32_t value)
{
	virtio_reg_write32(offset, virtio_reg_read32(offset) | value);
}

struct virtio_virtq *virtq_init(unsigned index)
{
	// Allocate a region for the virtqueue.
	paddr_t virtq_paddr = alloc_pages(align_up(sizeof(struct virtio_virtq), PAGE_SIZE) / PAGE_SIZE);

	struct virtio_virtq *vq = (struct virtio_virtq *) virtq_paddr;
	vq->queue_index = index;
	vq->used_index = (volatile uint16_t *) &vq->used.index;

	// Select the queue: Write the virtqueue index (first queue is 0).
	virtio_reg_write32(VIRTIO_REG_QUEUE_SEL, index);

	// Specify the queue size: Write the # of descriptors we'll use.
	virtio_reg_write32(VIRTIO_REG_QUEUE_NUM, VIRTQ_ENTRY_NUM);

	// Write the physical page frame number (not physical address!) of the queue.
	virtio_reg_write32(VIRTIO_REG_QUEUE_PFN, (uint32_t)((uintptr_t)virtq_paddr >> 12));

	return vq;
}

void virtio_blk_init(void)
{
	if (virtio_reg_read32(VIRTIO_REG_MAGIC) != 0x74726976)
		PANIC("virtio: invalid magic value");

	if (virtio_reg_read32(VIRTIO_REG_VERSION) != 1)
		PANIC("virtio: invalid version");

	if (virtio_reg_read32(VIRTIO_REG_DEVICE_ID) != VIRTIO_DEVICE_BLK)
		PANIC("virtio: invalid device id");

	// 1. Reset the device.
	virtio_reg_write32(VIRTIO_REG_DEVICE_STATUS, 0);

	// 2. Set the ACKNOWLEDGE status bit: We found the device.
	virtio_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_ACK);

	// 3. Set the DRIVER status bit: We know how to use the device.
	virtio_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_DRIVER);

	// 4. Set our page size: We use 4KB pages. This defines PFN (page frame number) calculation.
	virtio_reg_write32(VIRTIO_REG_PAGE_SIZE, PAGE_SIZE);

	// 5. Initialize a queue for disk read/write requests.
	blk_request_vq = virtq_init(0);

	// 6. Set the DRIVER_OK status bit: We can now use the device!
	virtio_reg_write32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_DRIVER_OK);

	// 7. Get the disk capacity.
	uint32_t low = virtio_reg_read32(VIRTIO_REG_DEVICE_CONFIG + 0);
	uint32_t high = virtio_reg_read32(VIRTIO_REG_DEVICE_CONFIG + 4);
	blk_capacity = (((uint64_t)high << 32) | low) * SECTOR_SIZE;
	printf("virtio-blk: capacity is %d bytes\n", (long)blk_capacity);

	// 8. Allocate a region to store requests to the device.
	blk_req_paddr = alloc_pages(align_up(sizeof(*blk_req), PAGE_SIZE) / PAGE_SIZE);
	blk_req = (struct virtio_blk_req *) blk_req_paddr;
}

// Notifies the device that there is a new request. `desc_index` is the index
// of the head descriptor of the new request.
void virtq_kick(struct virtio_virtq *vq, int desc_index) {
	vq->avail.ring[vq->avail.index % VIRTQ_ENTRY_NUM] = desc_index;
	vq->avail.index++;
	__sync_synchronize();
	virtio_reg_write32(VIRTIO_REG_QUEUE_NOTIFY, vq->queue_index);
	vq->last_used_index++;
}

// Returns whether there are requests being processed by the device.
bool virtq_is_busy(struct virtio_virtq *vq) {
	return vq->last_used_index != *vq->used_index;
}

// Reads/writes from/to virtio-blk device.
void read_write_disk(void *buf, unsigned sector, int is_write)
{
	if (sector >= blk_capacity / SECTOR_SIZE) {
		printf("virtio: tried to read/write sector=%d, but capacity is %d\n",
			(long)sector, (long)blk_capacity / SECTOR_SIZE);
		return;
	}

	// Construct the request according to the virtio-blk specification.
	blk_req->sector = sector;
	blk_req->type = is_write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
	if (is_write)
		memcpy(blk_req->data, buf, SECTOR_SIZE);

	// Construct the virtqueue descriptors (using 3 descriptors).
	struct virtio_virtq *vq = blk_request_vq;
	vq->descs[0].addr = blk_req_paddr;
	vq->descs[0].len = offsetof(struct virtio_blk_req, data);
	vq->descs[0].flags = VIRTQ_DESC_F_NEXT;
	vq->descs[0].next = 1;

	vq->descs[1].addr = blk_req_paddr + offsetof(struct virtio_blk_req, data);
	vq->descs[1].len = SECTOR_SIZE;
	vq->descs[1].flags = VIRTQ_DESC_F_NEXT | (is_write ? 0 : VIRTQ_DESC_F_WRITE);
	vq->descs[1].next = 2;

	vq->descs[2].addr = blk_req_paddr + offsetof(struct virtio_blk_req, status);
	vq->descs[2].len = sizeof(uint8_t);
	vq->descs[2].flags = VIRTQ_DESC_F_WRITE;

	// Notify the device that there is a new request.
	virtq_kick(vq, 0);

	// Wait until the device finishes processing.
	while (virtq_is_busy(vq))
		;

	// virtio-blk: If a non-zero value is returned, it's an error.
	if (blk_req->status != 0) {
		printf("virtio: warn: failed to read/write sector=%d status=%d\n",
			(long)sector, blk_req->status);
		return;
	}

	// For read operations, copy the data into the buffer.
	if (!is_write)
		memcpy(buf, blk_req->data, SECTOR_SIZE);
}

void kernel_main(void)
{
	// Initialize bss memory region to zero
	memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);

	printf("\n\n%s\n", KERNEL_NAME);
	printf("Version: %s\n", KERNEL_VERSION);

	WRITE_CSR(stvec, (uint64_t) kernel_entry);

	// Initialize virtio
	virtio_blk_init();

	// Trial to write a disk
	// 1. Initial read (Should show Lorem)
	char buf[SECTOR_SIZE + 1];
	read_write_disk(buf, 0, false);
	buf[SECTOR_SIZE] = '\0';
	printf("Before write: %s\n", buf);

	// 2. The Write
	memset(buf, 0, SECTOR_SIZE);
	strcpy(buf, "hello from kernel!!!\n");
	read_write_disk(buf, 0, true);
	printf("Write submitted...\n");

	// 3. Read it back to verify (The "Proof")
	memset(buf, 0, sizeof(buf));
	read_write_disk(buf, 0, false);
	buf[SECTOR_SIZE] = '\0';
	printf("After write: %s\n", buf);

	// Start process 0
	idle_proc = create_process(nullptr, 0);
	idle_proc->pid = 0;
	current_proc = idle_proc;

	// Start process shell
	size_t shell_size = (size_t) _binary_shell_bin_end - (size_t)_binary_shell_bin_start;
	create_process(_binary_shell_bin_start, shell_size);

	yield();

	for(;;) {
		__asm__ __volatile__("wfi");
	}
}

__attribute__((section(".text.boot")))
__attribute__((naked))
void boot(void)
{
	__asm__ __volatile__(
		"la sp, __stack_top\n"
		"j kernel_main\n"
	);
}
