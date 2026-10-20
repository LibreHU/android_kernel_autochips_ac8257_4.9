/* AC8257 (Linux 4.9): forced on every KernelSU-Next file, kvmalloc() & co. for kernels before 4.12 */
#ifndef __KSU_K49_H
#define __KSU_K49_H
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0)
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
static inline void *ksu_k49_kvmalloc(size_t size, gfp_t flags)
{
	void *buf = NULL;

	if (size <= 16 * PAGE_SIZE)
		buf = kmalloc(size, flags | __GFP_NOWARN);
	if (!buf)
		buf = (flags & __GFP_ZERO) ? vzalloc(size) : vmalloc(size);
	return buf;
}
#define kvmalloc(size, flags) ksu_k49_kvmalloc(size, flags)
#define kvzalloc(size, flags) ksu_k49_kvmalloc(size, (flags) | __GFP_ZERO)
#define kvcalloc(n, size, flags) ksu_k49_kvmalloc((n) * (size), (flags) | __GFP_ZERO)
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
/* kernel_read()/kernel_write() took their 4.14 signature (buf, count, &pos) after 4.9 */
#include <linux/fs.h>
extern ssize_t ksu_kernel_read_compat(struct file *p, void *buf, size_t count, loff_t *pos);
extern ssize_t ksu_kernel_write_compat(struct file *p, const void *buf, size_t count, loff_t *pos);
#define kernel_read(f, b, c, p) ksu_kernel_read_compat(f, b, c, p)
#define kernel_write(f, b, c, p) ksu_kernel_write_compat(f, b, c, p)
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 20, 0)
#include <linux/string.h>
/* strscpy_pad() appeared in 5.2 (4.14.222/4.19 stable) */
static inline ssize_t ksu_k49_strscpy_pad(char *dest, const char *src, size_t count)
{
	ssize_t res = strscpy(dest, src, count);

	if (res >= 0 && (size_t)res < count)
		memset(dest + res, 0, count - res);
	return res;
}
#define strscpy_pad(d, s, c) ksu_k49_strscpy_pad(d, s, c)
#endif
#endif
