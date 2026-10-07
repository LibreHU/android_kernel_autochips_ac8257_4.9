/* AC8257 (Linux 4.9): minimal <linux/overflow.h> (4.18), for the unsigned uses of this driver */
#ifndef __KSU_K49_OVERFLOW_H
#define __KSU_K49_OVERFLOW_H
#define check_add_overflow(a, b, d) ({		\
	typeof(a) __a = (a);			\
	typeof(b) __b = (b);			\
	typeof(d) __d = (d);			\
	*__d = __a + __b;			\
	*__d < __a;				\
})
#define struct_size(p, member, n)		\
	(sizeof(*(p)) + (size_t)(n) * sizeof(*(p)->member))
#endif
