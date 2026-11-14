// SPDX-License-Identifier: GPL-2.0
/*
 * LibreHU: test /init of the QEMU runs (tools/qemu/README.md). Static aarch64 program with raw system calls (no C
 * library): prints kernel information on the console, then powers off. Built by tools/qemu/build_init.sh.
 */
typedef unsigned long u64;
static long sc(long n, long a, long b, long c, long d, long e) {
	register long x8 asm("x8") = n, x0 asm("x0") = a, x1 asm("x1") = b, x2 asm("x2") = c, x3 asm("x3") = d,
		x4 asm("x4") = e;
	asm volatile("svc 0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3), "r"(x4) : "memory");
	return x0;
}
#define SYS_openat 56
#define SYS_close 57
#define SYS_read 63
#define SYS_write 64
#define SYS_mkdirat 34
#define SYS_mount 40
#define SYS_reboot 142
#define SYS_socket 198
#define SYS_nanosleep 101
#define AT_FDCWD -100
static long slen(const char *s) { long n = 0; while (s[n]) n++; return n; }
static void out(const char *s) { sc(SYS_write, 1, (long)s, slen(s), 0, 0); }
static void num(long v) { char b[24]; int i = 23; int neg = v < 0; u64 u = neg ? -v : v; b[i] = 0;
	do { b[--i] = '0' + u % 10; u /= 10; } while (u); if (neg) b[--i] = '-'; out(b + i); }
static void cat(const char *p) { char buf[4096]; long fd = sc(SYS_openat, AT_FDCWD, (long)p, 0, 0, 0);
	if (fd < 0) { out("open "); out(p); out(" failed "); num(fd); out("\n"); return; }
	for (;;) { long n = sc(SYS_read, fd, (long)buf, sizeof(buf), 0, 0); if (n <= 0) break; sc(SYS_write, 1, (long)buf, n, 0, 0); }
	sc(SYS_close, fd, 0, 0, 0, 0); }
void _start(void) {
	long fd = sc(SYS_openat, AT_FDCWD, (long)"/dev/console", 2, 0, 0);
	if (fd >= 0 && fd != 1) { /* dup3 to 1 and 2 */ sc(24, fd, 1, 0, 0, 0); sc(24, fd, 2, 0, 0, 0); }
	out("\n=== LIBREHU QEMU INIT: userspace reached ===\n");
	sc(SYS_mkdirat, AT_FDCWD, (long)"/proc", 0555, 0, 0);
	sc(SYS_mkdirat, AT_FDCWD, (long)"/sys", 0555, 0, 0);
	out("mount proc: "); num(sc(SYS_mount, (long)"proc", (long)"/proc", (long)"proc", 0, 0)); out("\n");
	out("mount sysfs: "); num(sc(SYS_mount, (long)"sysfs", (long)"/sys", (long)"sysfs", 0, 0)); out("\n");
	cat("/proc/version"); cat("/proc/cmdline");
	out("--- /proc/cpuinfo (head) via online cpus:\n"); cat("/sys/devices/system/cpu/online");
	out("--- meminfo:\n"); cat("/proc/meminfo");
	out("socket(AF_INET,SOCK_STREAM) as root: "); num(sc(SYS_socket, 2, 1, 0, 0, 0)); out("\n");
	out("--- filesystems:\n"); cat("/proc/filesystems");
	out("=== LIBREHU QEMU INIT: done, powering off ===\n");
	sc(SYS_reboot, 0xfee1dead, 672274793, 0x4321FEDC, 0, 0);
	for (;;) { long ts[2] = {1, 0}; sc(SYS_nanosleep, (long)ts, 0, 0, 0, 0); }
}
