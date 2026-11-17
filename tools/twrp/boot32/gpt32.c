// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2026 LibreHU
//
// gpt32 - UJC201 (AC8257, 128 GB eMMC) : GPT table for a 32 MiB boot partition, computed on the unit from its own
// primary GPT (the unique GUIDs and the disk size stay those of the unit). Reads only: the TWRP zips write the files
// it produces with dd and read them back.
//
//   gpt32 state <disk>                          print the layout of the primary GPT: orig, new or other
//   gpt32 make <disk> grow|shrink <pgpt> <sgpt> write the new primary GPT (protective MBR + header + entries) and the
//                                               new backup GPT (entries + header) to two files
//   gpt32 make <disk> same <pgpt> <sgpt>        the same two files for the current layout (orig or new): rewrites a
//                                               backup GPT lost by a power cut between the two writes
//
// Only entries 26 to 30 (boot, dtbo, tee1, tee2, vendor) change, and only their first / last LBA:
//   orig: boot 10 MiB @498 MiB, dtbo 8 MiB @508, tee1 5 MiB @516, tee2 7 MiB @521, vendor 2048 MiB @528
//   new:  boot 32 MiB @498 MiB, dtbo 1 MiB @530, tee1 5 MiB @531, tee2 7 MiB @536, vendor 2033 MiB @543
// system (entry 31) starts at 2576 MiB in both. Anything else (names, other LBAs, CRCs, header fields) is checked
// and refused if it differs.
//
// Two builds: hosted (libc, tests on the build machine: cc -DHOSTED) and freestanding static arm64 for TWRP (no libc,
// raw system calls): clang --target=aarch64-linux-gnu -ffreestanding -nostdlib -static -fuse-ld=lld.

#ifdef HOSTED
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define sys_open(p, f, m) open(p, f, m)
#define sys_read read
#define sys_write write
#define sys_close close
#define sys_lseek lseek
#define sys_exit exit
#else
typedef unsigned long size_t;
typedef long ssize_t;
typedef long off_t;
#define O_RDONLY 0
#define O_WRONLY 01
#define O_CREAT 0100
#define O_TRUNC 01000
#define SEEK_SET 0
#define SEEK_END 2
#define AT_FDCWD -100
static long sc(long n, long a, long b, long c, long d)
{
	register long x8 __asm__("x8") = n;
	register long x0 __asm__("x0") = a;
	register long x1 __asm__("x1") = b;
	register long x2 __asm__("x2") = c;
	register long x3 __asm__("x3") = d;
	__asm__ volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3) : "memory", "cc");
	return x0;
}
static int sys_open(const char *p, int f, int m) { return (int)sc(56, AT_FDCWD, (long)p, f, m); }
static ssize_t sys_read(int fd, void *b, size_t n) { return sc(63, fd, (long)b, (long)n, 0); }
static ssize_t sys_write(int fd, const void *b, size_t n) { return sc(64, fd, (long)b, (long)n, 0); }
static int sys_close(int fd) { return (int)sc(57, fd, 0, 0, 0); }
static off_t sys_lseek(int fd, off_t o, int w) { return sc(62, fd, o, w, 0); }
static void sys_exit(int c) { for (;;) sc(94, c, 0, 0, 0); }
void *memset(void *d, int c, size_t n) { unsigned char *p = d; while (n--) *p++ = (unsigned char)c; return d; }
void *memcpy(void *d, const void *s, size_t n) { unsigned char *p = d; const unsigned char *q = s; while (n--) *p++ = *q++; return d; }
int memcmp(const void *a, const void *b, size_t n)
{
	const unsigned char *p = a, *q = b;
	for (; n; n--, p++, q++)
		if (*p != *q)
			return *p - *q;
	return 0;
}
static int strcmp(const char *a, const char *b) { while (*a && *a == *b) a++, b++; return (unsigned char)*a - (unsigned char)*b; }
int main(int argc, char **argv);
void cstart(long *sp) { sys_exit(main((int)sp[0], (char **)(sp + 1))); }
__asm__(".text\n.global _start\n_start:\n\tmov x29, #0\n\tmov x0, sp\n\tbl cstart\n");
#endif

typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;

#define SECTOR 512
#define MIB_S 2048ULL			/* sectors per MiB */
#define MAX_ENTRIES_BYTES (128 * 128)
#define FIRST 25			/* 0-based index of boot (p26) */

static const char *const names[6] = { "boot", "dtbo", "tee1", "tee2", "vendor", "system" };
/* start and size in MiB of entries 26..30, and start of system (entry 31) */
static const u64 orig_l[5][2] = { { 498, 10 }, { 508, 8 }, { 516, 5 }, { 521, 7 }, { 528, 2048 } };
static const u64 new_l[5][2] = { { 498, 32 }, { 530, 1 }, { 531, 5 }, { 536, 7 }, { 543, 2033 } };
#define SYSTEM_START 2576

static u8 mbr[SECTOR], hdr[SECTOR], ent[MAX_ENTRIES_BYTES], bhdr[SECTOR], bent[MAX_ENTRIES_BYTES];
static u32 crc_table[256];

static void out(const char *s) { size_t n = 0; while (s[n]) n++; if (sys_write(1, s, n) < 0) n = 0; }
static void err(const char *s) { size_t n = 0; while (s[n]) n++; if (sys_write(2, s, n) < 0) n = 0; }
static void outnum(u64 v)
{
	char b[24]; int i = 23;
	b[i] = 0;
	do { b[--i] = (char)('0' + v % 10); v /= 10; } while (v);
	out(b + i);
}
static void die(const char *s) { err("gpt32: "); err(s); err("\n"); sys_exit(2); }

static u32 crc32(const u8 *p, size_t n)
{
	u32 c = 0xffffffffu;
	if (!crc_table[1])
		for (u32 i = 0; i < 256; i++) {
			u32 k = i;
			for (int j = 0; j < 8; j++)
				k = (k & 1) ? 0xedb88320u ^ (k >> 1) : k >> 1;
			crc_table[i] = k;
		}
	while (n--)
		c = crc_table[(c ^ *p++) & 0xff] ^ (c >> 8);
	return c ^ 0xffffffffu;
}
static u32 g32(const u8 *p) { return p[0] | p[1] << 8 | p[2] << 16 | (u32)p[3] << 24; }
static u64 g64(const u8 *p) { return g32(p) | (u64)g32(p + 4) << 32; }
static void p32(u8 *p, u32 v) { for (int i = 0; i < 4; i++) p[i] = (u8)(v >> (8 * i)); }
static void p64(u8 *p, u64 v) { for (int i = 0; i < 8; i++) p[i] = (u8)(v >> (8 * i)); }

static void pread_all(int fd, u64 off, void *buf, size_t n)
{
	u8 *b = buf;
	if (sys_lseek(fd, (off_t)off, SEEK_SET) != (off_t)off)
		die("seek");
	while (n) {
		ssize_t r = sys_read(fd, b, n);
		if (r <= 0)
			die("read");
		b += r; n -= (size_t)r;
	}
}
static void write_file(const char *path, const u8 *a, size_t na, const u8 *b, size_t nb, const u8 *c, size_t nc)
{
	int fd = sys_open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
		die("cannot create output file");
	const u8 *parts[3] = { a, b, c };
	size_t lens[3] = { na, nb, nc };
	for (int i = 0; i < 3; i++)
		for (size_t done = 0; done < lens[i];) {
			ssize_t w = sys_write(fd, parts[i] + done, lens[i] - done);
			if (w <= 0)
				die("write");
			done += (size_t)w;
		}
	sys_close(fd);
}

static u32 n_ent, ent_size;
static u64 alt_lba, ent_bytes, ent_sectors, disk_sectors;

/* header checks shared by the primary and the backup header */
static int header_ok(u8 *h)
{
	u32 hs = g32(h + 12), c = g32(h + 16);
	if (memcmp(h, "EFI PART", 8) || g32(h + 8) != 0x00010000 || hs != 92)
		return 0;
	p32(h + 16, 0);
	u32 calc = crc32(h, hs);
	p32(h + 16, c);
	return calc == c;
}

static int name_is(const u8 *e, const char *n)
{
	int i = 0;
	for (; n[i]; i++)
		if (e[56 + 2 * i] != (u8)n[i] || e[57 + 2 * i])
			return 0;
	return e[56 + 2 * i] == 0 && e[57 + 2 * i] == 0;
}

/* 1 = orig, 2 = new, 0 = other */
static int layout(const u8 *e)
{
	int is_orig = 1, is_new = 1;
	for (int i = 0; i < 6; i++)
		if (!name_is(e + (FIRST + i) * ent_size, names[i]))
			return 0;
	if (g64(e + (FIRST + 5) * ent_size + 32) != SYSTEM_START * MIB_S)
		return 0;
	for (int i = 0; i < 5; i++) {
		u64 s = g64(e + (FIRST + i) * ent_size + 32), l = g64(e + (FIRST + i) * ent_size + 40);
		if (s != orig_l[i][0] * MIB_S || l != (orig_l[i][0] + orig_l[i][1]) * MIB_S - 1)
			is_orig = 0;
		if (s != new_l[i][0] * MIB_S || l != (new_l[i][0] + new_l[i][1]) * MIB_S - 1)
			is_new = 0;
	}
	return is_orig ? 1 : is_new ? 2 : 0;
}

static void load(const char *disk, int *backup_ok)
{
	int fd = sys_open(disk, O_RDONLY, 0);
	if (fd < 0)
		die("cannot open the disk");
	off_t end = sys_lseek(fd, 0, SEEK_END);
	if (end <= 0 || end % SECTOR)
		die("disk size unknown");
	disk_sectors = (u64)end / SECTOR;
	pread_all(fd, 0, mbr, SECTOR);
	pread_all(fd, SECTOR, hdr, SECTOR);
	if (!header_ok(hdr))
		die("primary GPT header invalid (signature, revision, size or CRC)");
	n_ent = g32(hdr + 80); ent_size = g32(hdr + 84);
	ent_bytes = (u64)n_ent * ent_size;
	ent_sectors = (ent_bytes + SECTOR - 1) / SECTOR;
	alt_lba = g64(hdr + 32);
	if (g64(hdr + 24) != 1 || g64(hdr + 72) != 2 || ent_size != 128 || n_ent < FIRST + 6 || ent_bytes > MAX_ENTRIES_BYTES)
		die("primary GPT header: unexpected layout");
	if (alt_lba != disk_sectors - 1)
		die("backup GPT LBA is not the last sector of the disk");
	if (g64(hdr + 40) < 2 + ent_sectors || g64(hdr + 48) >= alt_lba - ent_sectors)
		die("primary GPT header: usable LBAs overlap the tables");
	pread_all(fd, 2 * SECTOR, ent, ent_bytes);
	if (crc32(ent, ent_bytes) != g32(hdr + 88))
		die("primary GPT entries CRC wrong");
	if (mbr[510] != 0x55 || mbr[511] != 0xaa || mbr[450] != 0xee)
		die("protective MBR missing");
	/* backup: same entries, header pointing back to LBA 1 */
	pread_all(fd, alt_lba * SECTOR, bhdr, SECTOR);
	*backup_ok = 0;
	if (header_ok(bhdr) && g64(bhdr + 24) == alt_lba && g64(bhdr + 32) == 1 &&
	    g64(bhdr + 72) == alt_lba - ent_sectors && g32(bhdr + 80) == n_ent && g32(bhdr + 84) == ent_size &&
	    g32(bhdr + 88) == g32(hdr + 88)) {
		pread_all(fd, (alt_lba - ent_sectors) * SECTOR, bent, ent_bytes);
		*backup_ok = !memcmp(bent, ent, ent_bytes);
	}
	sys_close(fd);
	for (u64 s = 0; s < ent_bytes; s += ent_size) {	/* no partition in the 5 MiB that move, except ours */
		u64 i = s / ent_size, fs = g64(ent + s + 32), ls = g64(ent + s + 40);
		if (i >= FIRST && i < FIRST + 5)
			continue;
		if (fs || ls)
			if (!(ls < 498 * MIB_S || fs >= SYSTEM_START * MIB_S))
				die("another partition lies in the 498..2576 MiB area");
	}
}

int main(int argc, char **argv)
{
	int bok;
	if (argc == 3 && !strcmp(argv[1], "state")) {
		load(argv[2], &bok);
		int l = layout(ent);
		out("state="); out(l == 1 ? "orig" : l == 2 ? "new" : "other");
		out(" backup="); out(bok ? "ok" : "bad");
		out(" disk_sectors="); outnum(disk_sectors);
		out(" sgpt_lba="); outnum(alt_lba - ent_sectors);
		out(" pgpt_sectors="); outnum(2 + ent_sectors);
		out(" sgpt_sectors="); outnum(ent_sectors + 1);
		out("\n");
		return l ? 0 : 1;
	}
	if (argc == 6 && !strcmp(argv[1], "make") &&
	    (!strcmp(argv[3], "grow") || !strcmp(argv[3], "shrink") || !strcmp(argv[3], "same"))) {
		int grow = !strcmp(argv[3], "grow"), same = !strcmp(argv[3], "same");
		load(argv[2], &bok);
		int l = layout(ent), want;
		if (same) {
			if (!l)
				die("the primary GPT is neither the original nor the 32 MiB layout");
			want = l;
		} else {
			if (l != (grow ? 1 : 2))
				die(grow ? "the primary GPT is not the original layout" : "the primary GPT is not the 32 MiB layout");
			const u64 (*to)[2] = grow ? new_l : orig_l;
			for (int i = 0; i < 5; i++) {
				u8 *e = ent + (FIRST + i) * ent_size;
				p64(e + 32, to[i][0] * MIB_S);
				p64(e + 40, (to[i][0] + to[i][1]) * MIB_S - 1);
			}
			want = grow ? 2 : 1;
		}
		if (layout(ent) != want)
			die("internal: new layout not recognised");
		p32(hdr + 88, crc32(ent, ent_bytes));
		p32(hdr + 16, 0);
		p32(hdr + 16, crc32(hdr, 92));
		/* backup header: same fields, LBAs swapped, entries just before it */
		memcpy(bhdr, hdr, SECTOR);
		p64(bhdr + 24, alt_lba);
		p64(bhdr + 32, 1);
		p64(bhdr + 72, alt_lba - ent_sectors);
		p32(bhdr + 16, 0);
		p32(bhdr + 16, crc32(bhdr, 92));
		if (!header_ok(hdr) || !header_ok(bhdr))
			die("internal: CRC");
		write_file(argv[4], mbr, SECTOR, hdr, SECTOR, ent, ent_sectors * SECTOR);
		write_file(argv[5], ent, ent_sectors * SECTOR, bhdr, SECTOR, 0, 0);
		out("sgpt_lba="); outnum(alt_lba - ent_sectors);
		out(" pgpt_sectors="); outnum(2 + ent_sectors);
		out(" sgpt_sectors="); outnum(ent_sectors + 1);
		out("\n");
		return 0;
	}
	err("usage: gpt32 state <disk> | gpt32 make <disk> grow|shrink|same <pgpt_out> <sgpt_out>\n");
	return 2;
}
