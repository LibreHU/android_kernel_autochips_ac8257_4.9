#include "rsv_cpu_notifier.h"

static BLOCKING_NOTIFIER_HEAD(rsv_cpu_notifier_list);

int rsv_cpu_register_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&rsv_cpu_notifier_list, nb);
}

int rsv_cpu_unregister_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&rsv_cpu_notifier_list, nb);
}

int rsv_cpu_notifier_call_chain(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(&rsv_cpu_notifier_list, val, v);
}
