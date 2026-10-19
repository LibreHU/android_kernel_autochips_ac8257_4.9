/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <linux/io.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/debugfs.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/kernel.h>
#include <linux/memblock.h>

#include "cli.h"

static cmd_block *command_list;
static const cmd_block *command_current;
/* a linear array of statically defined command blocks,
 * defined in the linker script.
 */
extern cmd_block __commands_start;
extern cmd_block __commands_end;

static int cmd_cd(int argc, const cmd_args *argv);
static int cmd_ls(int argc, const cmd_args *argv);
static int cmd_help(int argc, const cmd_args *argv);
static int cmd_test(int argc, const cmd_args *argv);

STATIC_COMMAND_START
	{ "cd",   "change the current block", 			&cmd_cd },
	{ "ls",   "list commands of the current block", &cmd_ls },
	{ "help", "this list",							&cmd_help },
	{ "test", "test the command processor", 		&cmd_test },
STATIC_COMMAND_END(home);

static int cli_cmd_init(void)
{
	/* add all the statically defined commands to the list */
	cmd_block *block;

	for (block = &__commands_start; block != &__commands_end; block++) {
		console_register_commands(block);
	}

	command_current = &_cmd_block_home;
	return 0;
}

static const cmd_block *match_block(const char *block_name)
{
	cmd_block *block;
	BUG_ON(block_name == NULL);

	for (block = command_list; block != NULL; block = block->next) {
		//match block;
		if (strcmp(block->name, block_name) == 0) {
			return block;
		}
	}

	return NULL;
}

static const cmd *match_command(const cmd_block *block, const char *cmd_name)
{
	size_t i;
	const cmd *curr_cmd;

	BUG_ON(block == NULL);
	BUG_ON(cmd_name == NULL);

	//First, match xxx block's cmd
	curr_cmd = block->list;

	for (i = 0; i < block->count; i++) {
		if (strcmp(cmd_name, curr_cmd[i].cmd_str) == 0) {
			return &curr_cmd[i];
		}
	}

	//Second, match the main block's cmd
	curr_cmd = _cmd_block_home.list;

	for (i = 0; i < _cmd_block_home.count; i++) {
		if (strcmp(cmd_name, curr_cmd[i].cmd_str) == 0) {
			return &curr_cmd[i];
		}
	}


	return NULL;
}

static int tokenize_command(char *buffer, cmd_args *args, int arg_count)
{
	int pos;
	int arg;
	bool finished;
	enum {
		INITIAL = 0,
		IN_SPACE,
		IN_TOKEN
	} state;

	pos = 0;
	arg = 0;
	state = INITIAL;
	finished = false;

	for (;;) {
		char c = buffer[pos];

		if (c == '\0')
			finished = true;

		//pr_err("c 0x%hhx state %d arg %d pos %d\n", c, state, arg, pos);

		switch (state) {
		case INITIAL:
			if (isspace(c)) {
				state = IN_SPACE;
			} else {
				state = IN_TOKEN;
				args[arg].str = &buffer[pos];
			}
			break;
		case IN_TOKEN:
			if (finished) {
				arg++;
				goto done;
			}
			if (isspace(c)) {
				arg++;
				buffer[pos] = 0;
				/* are we out of tokens? */
				if (arg == arg_count)
					goto done;
				state = IN_SPACE;
			}
			pos++;
			break;
		case IN_SPACE:
			if (finished)
				goto done;
			if (!isspace(c)) {
				state = IN_TOKEN;
				args[arg].str = &buffer[pos];
			}
			pos++;
			break;
		}
	}

done:
	return arg;
}

static void convert_args(int argc, cmd_args *argv)
{
	int i, ret;

	for (i = 0; i < argc; i++) {
		ret = kstrtoul(argv[i].str, 0, &(argv[i].u));
		if (ret < 0)
			continue;

		ret = kstrtol(argv[i].str, 0, &(argv[i].i));
		if (ret < 0)
			continue;
	
	}
}

static int console_loop(const char *buffer, size_t len)
{
	//int i;
	int result = -EINVAL;
	int argc;
	cmd_args args[16];
	cmd_args *p;
	const cmd_block *block;
	const cmd *command;

	if (len <= 0)
		return result;

	//pr_err("line = '%s'\n", buffer);

	/* tokenize the line */
	argc = tokenize_command((char *)buffer, args, 16);
	if (argc <= 0) {
		pr_err("syntax error\n");
		return result;
	}
	p = &args[0];

	block = match_block(args[0].str);
	if (block) {
		argc--;
		p = &args[1];
		command = match_command(block, p->str);
	} else {
		command = match_command(command_current, p->str);
	}

	if (!command) {
		pr_err("command not found\n");
		return result;
	}

	//pr_err("after tokenize: argc %d\n", argc);
	//for (i = 0; i < argc; i++)
	//	pr_err("%d: '%s'\n", i, args[i].str);

	/* convert the args */
	convert_args(argc, p);

	result = command->cmd_callback(argc, p);

	return result;
	// XXX do something with the result
}

int console_run_command(const char *string)
{
	const cmd *command;
	int result;

	BUG_ON(string == NULL);

	command = match_command(&_cmd_block_home, string);
	if (!command)
		return -1;

	result = command->cmd_callback(0, NULL);

	return result;
}

void console_register_commands(cmd_block *block)
{
	BUG_ON(block == NULL);
	BUG_ON(block->next);

	block->next = command_list;
	command_list = block;
}


/*
 * Home command list:
 *  ls, cd, help, test
 */
static int cmd_ls(int argc, const cmd_args *argv)
{
	size_t i;

	pr_err("Block %s Cmds:\n", command_current->name);

	for (i = 0; i < command_current->count; i++) {
		pr_err("\t%-16s: %s\n", command_current->list[i].cmd_str,
			command_current->list[i].help_str ? command_current->list[i].help_str : "");
	}

	return 0;
}

static int cmd_cd(int argc, const cmd_args *argv)
{
	const cmd_block *block;
	const char *block_name;
	switch (argc) {
	case 1:
		command_current = &_cmd_block_home;
		break;

	case 2:
		block_name = argv[1].str;
		block = match_block(block_name);
		if (!block) {
			pr_err("Bad block name: %s\n", block_name);
			return 0;
		}

		pr_err("cd block: %s\n", block->name);
		command_current = block;
		break;
	}

	return 0;
}

static int cmd_help(int argc, const cmd_args *argv)
{
	size_t i;
	const cmd_block *block = NULL;
	const char *block_name;

	if (argc == 2) {
		block_name = argv[1].str;
		block = match_block(block_name);
	}

	if (!block) {
		if (command_current == &_cmd_block_home) {
			pr_err("\nBlocks:\n");
			for (block = command_list; block != NULL; block = block->next) {
				pr_err("\t%s\n", block->name);
			}
		}

		block = command_current;
	}

	pr_err("\nBlock %s Cmds:\n", block->name);
	for (i = 0; i < block->count; i++) {
		pr_err("\t%-16s: %s\n", block->list[i].cmd_str,
				block->list[i].help_str ? block->list[i].help_str : "");
	}

	return 0;
}

static int cmd_test(int argc, const cmd_args *argv)
{
	int i;

	pr_err("argc %d, argv %p\n", argc, argv);
	for (i = 0; i < argc; i++)
		pr_err("\t%d: str '%s', i %ld, u %ld\n", i,
			argv[i].str, argv[i].i, argv[i].u);

	return 0;
}

struct kobject *cli_kobj;
EXPORT_SYMBOL_GPL(cli_kobj);

static ssize_t commands_show(struct kobject *kobj, struct kobj_attribute *attr,
					char *buf)
{
	console_run_command("help");

	return 0;
}

static ssize_t commands_store(struct kobject *kobj, struct kobj_attribute *attr,
					const char *buf, size_t n)
{
	char *p;
	int len;

	p = memchr(buf, '\n', n);
	len = p ? p - buf : n;

	console_loop(buf, len);

	return n;
}

#if 0
static struct kobj_attribute commands_attr = {
	.attr = {
		.name = "commands",
		.mode = 0644,
	},
	.show	= commands_show,
	.store	= commands_store,
};

static struct attribute *g[] = {
	&commands_attr.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = g,
};
#else
static struct kobj_attribute commands_attr = __ATTR(commands,
			0644,
			commands_show,
			commands_store);
#endif

static int __init cli_init(void)
{
	int ret = 0;

	cli_kobj = kobject_create_and_add("cli", NULL);
	if (!cli_kobj)
		return -ENOMEM;
#if 0
	ret = sysfs_create_group(cli_kobj, &attr_group);
#else
	ret = sysfs_create_file(cli_kobj, &commands_attr.attr);
#endif

	if (ret)
		pr_err("[BS] Fail to create file /sys/cli/commands");

	cli_cmd_init();

	return ret;
}

static void __exit cli_exit(void)
{
	kobject_put(cli_kobj);
}

module_init(cli_init);
module_exit(cli_exit);

void __iomem *io_base;
unsigned long MEM_BASE;
unsigned long MEM_END;

static int __init cli_io_init(void)
{
	io_base = ioremap(IO_BASE, 0x08FFFFFF);
	if (!io_base) {
		pr_err("ioremap failed\n");
	}

	MEM_BASE = memblock_start_of_DRAM();
	MEM_END = memblock_end_of_DRAM();

	return 0;
}
arch_initcall(cli_io_init);

MODULE_DESCRIPTION("ATC cli Driver");
MODULE_LICENSE("GPL");
