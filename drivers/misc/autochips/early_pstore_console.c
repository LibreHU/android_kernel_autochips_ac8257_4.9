/*
 * Bring-up aid: copies the kernel log into the pstore console zone from the start of setup_arch(), so that a
 * kernel dying before ramoops registers (postcore_initcall, about 0.15 s on the stock kernel) still
 * leaves its log in /sys/fs/pstore/console-ramoops and /proc/last_kmsg of the next (stock) boot.
 *
 * The zone is written in the persistent_ram format (fs/pstore/ram_core.c, no ECC) at the place
 * ramoops puts its first console zone with the stock layout: CONFIG_PSTORE_MEM_ADDR, then the dump
 * records (4 KiB each), then the console zone (the stock ramoops parameters, read from the stock
 * image, give 0x47cdf000). Until paging_init() the zone is reached through an early_memremap()
 * mapping, then through the linear map. Writes are cleaned to DRAM after each line, since DRAM
 * survives the watchdog reset but caches do not.
 *
 * The console stops when ramoops registers (ac8257_early_pstore_console_stop()); from there the real
 * pstore console takes over. This program is free software; GPL v2.
 */
#include <linux/console.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <asm/cacheflush.h>
#include <asm/early_ioremap.h>
#include <asm/memory.h>

#define PERSISTENT_RAM_SIG	0x43474244	/* DBGC, as in ram_core.c */
#define RAMOOPS_RECORD_SIZE	4096		/* ram.c defaults (MIN_MEM_SIZE) */
#define RAMOOPS_FTRACE_SIZE	4096

struct persistent_ram_buffer {
	u32 sig;
	u32 start;
	u32 size;
	u8 data[0];
};

static struct persistent_ram_buffer *buf;
static u32 data_size;
static phys_addr_t zone_pa;
static bool early_mapped;

static void ac8257_epc_copy(const char *s, u32 count)
{
	u32 start = buf->start;

	while (count) {
		u32 n = min(count, data_size - start);

		memcpy(buf->data + start, s, n);
		__flush_dcache_area(buf->data + start, n);
		s += n;
		count -= n;
		start = (start + n) % data_size;
		if (buf->size < data_size)
			buf->size = min(buf->size + n, data_size);
	}
	buf->start = start;
	__flush_dcache_area(buf, sizeof(*buf));
}

static void ac8257_epc_write(struct console *con, const char *s, unsigned int count)
{
	if (buf)
		ac8257_epc_copy(s, count);
}

static struct console ac8257_epc = {
	.name	= "ac8257epc",
	.write	= ac8257_epc_write,
	.flags	= CON_PRINTBUFFER | CON_ENABLED | CON_ANYTIME | CON_BOOT,
	.index	= -1,
};

void __init ac8257_early_pstore_console_init(void)
{
	unsigned long dump = CONFIG_PSTORE_MEM_SIZE - 2 * CONFIG_PSTORE_CONSOLE_SIZE -
			     RAMOOPS_FTRACE_SIZE - CONFIG_PSTORE_PMSG_SIZE;
	phys_addr_t pa = CONFIG_PSTORE_MEM_ADDR + dump / RAMOOPS_RECORD_SIZE * RAMOOPS_RECORD_SIZE;
	static const char mark[] = "\n==== ac8257 early pstore console ====\n";

	zone_pa = pa;
	buf = early_memremap(pa, CONFIG_PSTORE_CONSOLE_SIZE);
	if (!buf)
		return;
	early_mapped = true;
	data_size = CONFIG_PSTORE_CONSOLE_SIZE - sizeof(*buf);
	/* Keep what the previous boot left (it shows before this boot's log), unless invalid. */
	if (buf->sig != PERSISTENT_RAM_SIG || buf->size > data_size || buf->start > buf->size) {
		buf->sig = PERSISTENT_RAM_SIG;
		buf->start = 0;
		buf->size = 0;
	}
	ac8257_epc_copy(mark, sizeof(mark) - 1);
	register_console(&ac8257_epc);
}

/* After paging_init(): move from the early mapping to the linear map. */
void __init ac8257_early_pstore_console_remap(void)
{
	void *early = buf;

	if (!early_mapped)
		return;
	buf = phys_to_virt(zone_pa);
	early_mapped = false;
	early_memunmap(early, CONFIG_PSTORE_CONSOLE_SIZE);
}

void ac8257_early_pstore_console_stop(void)
{
	if (!buf)
		return;
	unregister_console(&ac8257_epc);
	buf = NULL;
}
