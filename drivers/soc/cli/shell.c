/*
 * Copyright (c) 2015 Travis Geiselbrecht
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

#include "cli.h"

/* shell console hooks for manipulating the file system */
static int cmd_ls(int argc, const cmd_args *argv)
{
    printk("%s\n", __func__);
	return 0;
}

static int cmd_cd(int argc, const cmd_args *argv)
{
    printk("%s\n", __func__);
    return 0;
}

static int cmd_pwd(int argc, const cmd_args *argv)
{
    printk("%s\n", __func__);
	return 0;
}

static int cmd_mkdir(int argc, const cmd_args *argv)
{
    printk("%s\n", __func__);
	return 0;
}

static int cmd_mkfile(int argc, const cmd_args *argv)
{
    printk("%s\n", __func__);
	return 0;
}

static int cmd_rm(int argc, const cmd_args *argv)
{
    printk("%s\n", __func__);
	return 0;
}

static int cmd_stat(int argc, const cmd_args *argv)
{
    printk("%s\n", __func__);
	return 0;
}

static int cmd_cat(int argc, const cmd_args *argv)
{
    printk("%s\n", __func__);
	return 0;
}

STATIC_COMMAND_START
	{"ls", "dir listing", &cmd_ls},
	{"cd", "change dir", &cmd_cd},
	{"pwd", "print working dir", &cmd_pwd},
	{"mkdir", "make dir", &cmd_mkdir},
	{"mkfile", "make file", &cmd_mkfile},
	{"rm", "remove file", &cmd_rm},
	{"stat", "stat file", &cmd_stat},
	{"cat", "cat file", &cmd_cat},
STATIC_COMMAND_END(fs_shell);

