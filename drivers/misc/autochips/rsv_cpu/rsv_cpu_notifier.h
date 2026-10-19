#ifndef __RSV_CPU_NOTIFIER__
#define __RSV_CPU_NOTIFIER__
#include <linux/notifier.h>
#define RSV_CPU_EVENT_RELEASE 0x01

#define RSV_CPU_EVENT_MASK  0xFFFF
#define RSV_CPU_EVENT_SHIFT 16

#define RSV_CPU_EVENT(event, val) \
	(((event & RSV_CPU_EVENT_MASK) << RSV_CPU_EVENT_SHIFT) | (val & RSV_CPU_EVENT_MASK))

int rsv_cpu_register_client(struct notifier_block *nb);

int rsv_cpu_unregister_client(struct notifier_block *nb);

int rsv_cpu_notifier_call_chain(unsigned long val, void *v);
#endif
