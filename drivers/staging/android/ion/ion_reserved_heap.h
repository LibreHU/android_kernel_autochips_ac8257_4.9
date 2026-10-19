#ifndef __ION_RESERVED_HEAP_H__
#define __ION_RESERVED_HEAP_H__

struct ion_reserved_buffer_info {
	struct mutex lock;	/*mutex lock on fb buffer */
	int module_id;
	unsigned int security;
	unsigned int coherent;
	void *VA;
	unsigned int MVA;
	unsigned int FIXED_MVA;
	unsigned long iova_start;
	unsigned long iova_end;
	ion_phys_addr_t priv_phys;
	struct ion_mm_buf_debug_info dbg_info;
};

#endif
