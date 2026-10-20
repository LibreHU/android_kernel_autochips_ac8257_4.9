/*
 * rc-jancar.c - keytable of the IR receiver of the Jancar UJC201 (gpio-ir-receiver, "rc-jancar" in its device tree)
 *
 * Copyright (C) 2026 LibreHU
 *
 * Read back from the stock UJC201 kernel (4.9.117+ #25), whose source Jancar did not publish: an identity table,
 * NEC scancodes 0x001 to 0x2ff give the key codes 1 to 767. Jancar's Android side maps these key codes to its
 * functions.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <media/rc-map.h>
#include <linux/module.h>

#define JANCAR_KEYS	0x2ff

/* Same table as the stock one, filled at init instead of listing 767 identical pairs */
static struct rc_map_table jancar[JANCAR_KEYS];

static struct rc_map_list jancar_map = {
	.map = {
		.scan    = jancar,
		.size    = ARRAY_SIZE(jancar),
		.rc_type = RC_TYPE_NEC,
		.name    = "rc-jancar",
	}
};

static int __init init_rc_map_jancar(void)
{
	int i;

	for (i = 0; i < JANCAR_KEYS; i++) {
		jancar[i].scancode = i + 1;
		jancar[i].keycode = i + 1;
	}
	return rc_map_register(&jancar_map);
}

static void __exit exit_rc_map_jancar(void)
{
	rc_map_unregister(&jancar_map);
}

module_init(init_rc_map_jancar)
module_exit(exit_rc_map_jancar)

MODULE_LICENSE("GPL");
