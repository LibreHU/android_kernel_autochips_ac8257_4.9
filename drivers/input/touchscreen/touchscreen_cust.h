#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/delay.h>

struct touuch_data {
	unsigned int serializer_addr;
	unsigned int deserializer_addr;
	unsigned int touchpad_addr;
};
