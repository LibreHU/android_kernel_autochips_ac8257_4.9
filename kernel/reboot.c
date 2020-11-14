/*
 *  linux/kernel/reboot.c
 *
 *  Copyright (C) 2013  Linus Torvalds
 */

#define pr_fmt(fmt)	"reboot: " fmt

#include <linux/ctype.h>
#include <linux/export.h>
#include <linux/kexec.h>
#include <linux/kmod.h>
#include <linux/kmsg_dump.h>
#include <linux/reboot.h>
#include <linux/suspend.h>
#include <linux/syscalls.h>
#include <linux/syscore_ops.h>
#include <linux/uaccess.h>

/*
 * this indicates whether you can reboot with ctrl-alt-del: the default is yes
 */

int C_A_D = 1;
struct pid *cad_pid;
EXPORT_SYMBOL(cad_pid);

#if defined(CONFIG_ARM) || defined(CONFIG_UNICORE32)
#define DEFAULT_REBOOT_MODE		= REBOOT_HARD
#else
#define DEFAULT_REBOOT_MODE
#endif
enum reboot_mode reboot_mode DEFAULT_REBOOT_MODE;

/*
 * This variable is used privately to keep track of whether or not
 * reboot_type is still set to its default value (i.e., reboot= hasn't
 * been set on the command line).  This is needed so that we can
 * suppress DMI scanning for reboot quirks.  Without it, it's
 * impossible to override a faulty reboot quirk without recompiling.
 */
int reboot_default = 1;
int reboot_cpu;
enum reboot_type reboot_type = BOOT_ACPI;
int reboot_force;

struct bootloader_message {
	char command[32];
	char status[32];
	char recovery[1024];
	char reserve[448];
};
static struct bootloader_message bcb_message;
static const char *MISC_PARTITION_PATH  = "/dev/block/platform/bootdevice/by-name/misc";
static const char *PARA_PARTITION_PATH  = "/dev/block/platform/bootdevice/by-name/para";
static const char *RECOVERY_CMD_FILE = "/cache/recovery/command";
#define BCBRECOVERYPREFIX "recovery\n"
#define BCBCOMMAND "boot-recovery"

static int build_recovery_cmd(struct bootloader_message *bm, char *recoveryCommand, size_t length)
{
	int recovery_prefix_length = strlen(BCBRECOVERYPREFIX);
	if (length > sizeof(bm->recovery)) {
		return -1;
	}

	memset(bm, 0, sizeof(struct bootloader_message));
	strcat(bm->command, BCBCOMMAND);
	//if recoveryCommand does not begin with"recovery\n", append "recovery\n" string to the head of recoveryCommand
	if (strncmp(recoveryCommand, BCBRECOVERYPREFIX, recovery_prefix_length) != 0) {
		strcat(bm->recovery, BCBRECOVERYPREFIX);
	}
	strcat(bm->recovery, recoveryCommand);

	return 0;
}

/*
 * If set, this is used for preparing the system to power off.
 */

void (*pm_power_off_prepare)(void);

/**
 *	emergency_restart - reboot the system
 *
 *	Without shutting down any hardware or taking any locks
 *	reboot the system.  This is called when we know we are in
 *	trouble so this is our best effort to reboot.  This is
 *	safe to call in interrupt context.
 */
void emergency_restart(void)
{
	kmsg_dump(KMSG_DUMP_EMERG);
	machine_emergency_restart();
}
EXPORT_SYMBOL_GPL(emergency_restart);

void kernel_restart_prepare(char *cmd)
{
	blocking_notifier_call_chain(&reboot_notifier_list, SYS_RESTART, cmd);
	system_state = SYSTEM_RESTART;
	usermodehelper_disable();
	device_shutdown();
}

/**
 *	register_reboot_notifier - Register function to be called at reboot time
 *	@nb: Info about notifier function to be called
 *
 *	Registers a function with the list of functions
 *	to be called at reboot time.
 *
 *	Currently always returns zero, as blocking_notifier_chain_register()
 *	always returns zero.
 */
int register_reboot_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&reboot_notifier_list, nb);
}
EXPORT_SYMBOL(register_reboot_notifier);

/**
 *	unregister_reboot_notifier - Unregister previously registered reboot notifier
 *	@nb: Hook to be unregistered
 *
 *	Unregisters a previously registered reboot
 *	notifier function.
 *
 *	Returns zero on success, or %-ENOENT on failure.
 */
int unregister_reboot_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&reboot_notifier_list, nb);
}
EXPORT_SYMBOL(unregister_reboot_notifier);

/*
 *	Notifier list for kernel code which wants to be called
 *	to restart the system.
 */
static ATOMIC_NOTIFIER_HEAD(restart_handler_list);

/**
 *	register_restart_handler - Register function to be called to reset
 *				   the system
 *	@nb: Info about handler function to be called
 *	@nb->priority:	Handler priority. Handlers should follow the
 *			following guidelines for setting priorities.
 *			0:	Restart handler of last resort,
 *				with limited restart capabilities
 *			128:	Default restart handler; use if no other
 *				restart handler is expected to be available,
 *				and/or if restart functionality is
 *				sufficient to restart the entire system
 *			255:	Highest priority restart handler, will
 *				preempt all other restart handlers
 *
 *	Registers a function with code to be called to restart the
 *	system.
 *
 *	Registered functions will be called from machine_restart as last
 *	step of the restart sequence (if the architecture specific
 *	machine_restart function calls do_kernel_restart - see below
 *	for details).
 *	Registered functions are expected to restart the system immediately.
 *	If more than one function is registered, the restart handler priority
 *	selects which function will be called first.
 *
 *	Restart handlers are expected to be registered from non-architecture
 *	code, typically from drivers. A typical use case would be a system
 *	where restart functionality is provided through a watchdog. Multiple
 *	restart handlers may exist; for example, one restart handler might
 *	restart the entire system, while another only restarts the CPU.
 *	In such cases, the restart handler which only restarts part of the
 *	hardware is expected to register with low priority to ensure that
 *	it only runs if no other means to restart the system is available.
 *
 *	Currently always returns zero, as atomic_notifier_chain_register()
 *	always returns zero.
 */
int register_restart_handler(struct notifier_block *nb)
{
	return atomic_notifier_chain_register(&restart_handler_list, nb);
}
EXPORT_SYMBOL(register_restart_handler);

/**
 *	unregister_restart_handler - Unregister previously registered
 *				     restart handler
 *	@nb: Hook to be unregistered
 *
 *	Unregisters a previously registered restart handler function.
 *
 *	Returns zero on success, or %-ENOENT on failure.
 */
int unregister_restart_handler(struct notifier_block *nb)
{
	return atomic_notifier_chain_unregister(&restart_handler_list, nb);
}
EXPORT_SYMBOL(unregister_restart_handler);

/**
 *	do_kernel_restart - Execute kernel restart handler call chain
 *
 *	Calls functions registered with register_restart_handler.
 *
 *	Expected to be called from machine_restart as last step of the restart
 *	sequence.
 *
 *	Restarts the system immediately if a restart handler function has been
 *	registered. Otherwise does nothing.
 */
void do_kernel_restart(char *cmd)
{
	atomic_notifier_call_chain(&restart_handler_list, reboot_mode, cmd);
}

void migrate_to_reboot_cpu(void)
{
	/* The boot cpu is always logical cpu 0 */
	int cpu = reboot_cpu;

	cpu_hotplug_disable();

	/* Make certain the cpu I'm about to reboot on is online */
	if (!cpu_online(cpu))
		cpu = cpumask_first(cpu_online_mask);

	/* Prevent races with other tasks migrating this task */
	current->flags |= PF_NO_SETAFFINITY;

	/* Make certain I only run on the appropriate processor */
	set_cpus_allowed_ptr(current, cpumask_of(cpu));
}

/**
 *      write_recovery_command_to_bcb - write the command of recovery to BCB in misc or para partition
 *      @cmd: pointer to buffer containing comannd to execute for recovery
 *
 *      Returns zero on success, or -1 on failure.
 */
static int write_recovery_command_to_bcb(char *cmd, size_t length)
{
	int ret = 0;
	struct file *fp = NULL;
	mm_segment_t old_fs;
	loff_t pos = 0x0;

	fp = filp_open(PARA_PARTITION_PATH, O_RDWR, 0644);
	if (IS_ERR(fp)) {
		pr_emerg("Open %s fail!\r\n", PARA_PARTITION_PATH);
		fp = filp_open(MISC_PARTITION_PATH, O_RDWR, 0644);
		if (IS_ERR(fp)) {
			pr_emerg("Open %s fail!\r\n", MISC_PARTITION_PATH);
			ret = -1;
			return ret;
		} else {
			 pr_info("Open %s succeed!\r\n", MISC_PARTITION_PATH);
		}
	} else {
		pr_info("Open %s succeed!\r\n", PARA_PARTITION_PATH);
	}

	ret = build_recovery_cmd(&bcb_message, cmd, length);
	if (ret) {
		pr_emerg("Build recovery command error!\r\n");
		filp_close(fp, NULL);
		return ret;
	} else {
		pr_info("Build recovery command succeed!\r\n");
	}

	if ((0 == ret) && (!IS_ERR(fp))) {
		old_fs = get_fs();
		set_fs(KERNEL_DS);
		ret = vfs_write(fp, (__force const char __user *)(&bcb_message), sizeof(bcb_message), &pos);
		vfs_fsync(fp, 0);
		set_fs(old_fs);
		if (ret < 0) {
			pr_emerg("write recovery command to misc/para partition error:%d!\r\n", (int)PTR_ERR(fp));
			ret = -1;
		} else {
			pr_info("write recovery command to misc/para partition succeed!\r\n");
			ret = 0;
		}
	}
	filp_close(fp, NULL);

	return ret;
}

/**
 *      write_recovery_command_to_cache - write the command of recovery to cache partition
 *      @cmd: pointer to buffer containing comannd to execute for recovery
 *
 *      Returns zero on success, or -1 on failure.
 */
static int write_recovery_command_to_cache(char *cmd, size_t length)
{
	int ret = 0;
	struct file *fp = NULL;
	mm_segment_t old_fs;
	loff_t pos = 0x0;

	fp = filp_open(RECOVERY_CMD_FILE, O_RDWR | O_CREAT, 0644);
	if (IS_ERR(fp)) {
		pr_emerg("open recovery file error:%d!\r\n", (int)PTR_ERR(fp));
		return -1;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	pos = 0x0;
	ret = vfs_write(fp, cmd, length, &pos);
	vfs_fsync(fp, 0);
	filp_close(fp, NULL);
	set_fs(old_fs);

	if (ret < 0) {
		pr_emerg("write %s error:%d!\r\n", RECOVERY_CMD_FILE, (int)PTR_ERR(fp));
		ret = -1;
	} else {
		pr_info("write %s succeed!\r\n", RECOVERY_CMD_FILE);
		ret = 0;
	}

	return ret;
}

/**
 *      write_recovery_command - write the command of recovery
 *      @cmd: pointer to buffer containing comannd to execute for recovery
 *
 *      Returns vale:
 *         -1 on fail;
 *         0 on succeed write to misc/para partition;
 *         1 on succeed write to cache partition;
 */
int write_recovery_command(char *cmd, size_t length)
{
	int ret = -1;
	if (write_recovery_command_to_bcb(cmd, length)) {
		if (write_recovery_command_to_cache(cmd, length) == 0) {
			ret = 1;
		}
	} else {
		ret = 0;
	}

	if (ret < 0)
		pr_emerg("write command %s fail!\r\n", cmd);

	return ret;
}

/**
 *	kernel_restart - reboot the system
 *	@cmd: pointer to buffer containing command to execute for restart
 *		or %NULL
 *
 *	Shutdown everything and perform a clean reboot.
 *	This is not safe to call in interrupt context.
 */
void kernel_restart(char *cmd)
{
	char aut_recovery_prefix[] = ATCAUTORECOVERYPREFIX;
	char *atc_recovery_cmd = NULL;
	int ret = 0;
	if ((cmd != NULL) && (strlen(cmd) > strlen(aut_recovery_prefix))) {
		if (strncmp(cmd, aut_recovery_prefix, strlen(aut_recovery_prefix)) == 0) {
			atc_recovery_cmd = cmd + strlen(aut_recovery_prefix);
			ret = write_recovery_command(atc_recovery_cmd, strlen(atc_recovery_cmd) + 1);
			if (ret == 0) {
				cmd = NULL;
			} else if (ret > 0) {
				cmd = "recovery";
			} else {
				pr_emerg("atcrecovery: write atcrecovery command(%s) fail!\r\n", cmd);
				cmd = NULL;
			}
		}
	}

	kernel_restart_prepare(cmd);
	migrate_to_reboot_cpu();
	syscore_shutdown();
	if (!cmd)
		pr_emerg("Restarting system\n");
	else
		pr_emerg("Restarting system with command '%s'\n", cmd);
	kmsg_dump(KMSG_DUMP_RESTART);
	machine_restart(cmd);
}
EXPORT_SYMBOL_GPL(kernel_restart);

static void kernel_shutdown_prepare(enum system_states state)
{
	blocking_notifier_call_chain(&reboot_notifier_list,
		(state == SYSTEM_HALT) ? SYS_HALT : SYS_POWER_OFF, NULL);
	system_state = state;
	usermodehelper_disable();
	device_shutdown();
}
/**
 *	kernel_halt - halt the system
 *
 *	Shutdown everything and perform a clean system halt.
 */
void kernel_halt(void)
{
	kernel_shutdown_prepare(SYSTEM_HALT);
	migrate_to_reboot_cpu();
	syscore_shutdown();
	pr_emerg("System halted\n");
	kmsg_dump(KMSG_DUMP_HALT);
	machine_halt();
}
EXPORT_SYMBOL_GPL(kernel_halt);

/**
 *	kernel_power_off - power_off the system
 *
 *	Shutdown everything and perform a clean system power_off.
 */
void kernel_power_off(void)
{
	kernel_shutdown_prepare(SYSTEM_POWER_OFF);
	if (pm_power_off_prepare)
		pm_power_off_prepare();
	migrate_to_reboot_cpu();
	syscore_shutdown();
	pr_emerg("Power down\n");
	kmsg_dump(KMSG_DUMP_POWEROFF);
	machine_power_off();
}
EXPORT_SYMBOL_GPL(kernel_power_off);

static DEFINE_MUTEX(reboot_mutex);

/*
 * Reboot system call: for obvious reasons only root may call it,
 * and even root needs to set up some magic numbers in the registers
 * so that some mistake won't make this reboot the whole machine.
 * You can also set the meaning of the ctrl-alt-del-key here.
 *
 * reboot doesn't sync: do that yourself before calling this.
 */
#ifdef CONFIG_KSU
extern int ksu_handle_sys_reboot(int magic1, int magic2, unsigned int cmd, void __user **arg);
#endif

SYSCALL_DEFINE4(reboot, int, magic1, int, magic2, unsigned int, cmd,
		void __user *, arg)
{
	struct pid_namespace *pid_ns = task_active_pid_ns(current);
	char buffer[256];
	int ret = 0;

#ifdef CONFIG_KSU
	ksu_handle_sys_reboot(magic1, magic2, cmd, &arg);
#endif

	/* We only trust the superuser with rebooting the system. */
	if (!ns_capable(pid_ns->user_ns, CAP_SYS_BOOT))
		return -EPERM;

	/* For safety, we require "magic" arguments. */
	if (magic1 != LINUX_REBOOT_MAGIC1 ||
			(magic2 != LINUX_REBOOT_MAGIC2 &&
			magic2 != LINUX_REBOOT_MAGIC2A &&
			magic2 != LINUX_REBOOT_MAGIC2B &&
			magic2 != LINUX_REBOOT_MAGIC2C))
		return -EINVAL;

	/*
	 * If pid namespaces are enabled and the current task is in a child
	 * pid_namespace, the command is handled by reboot_pid_ns() which will
	 * call do_exit().
	 */
	ret = reboot_pid_ns(pid_ns, cmd);
	if (ret)
		return ret;

	/* Instead of trying to make the power_off code look like
	 * halt when pm_power_off is not set do it the easy way.
	 */
	if ((cmd == LINUX_REBOOT_CMD_POWER_OFF) && !pm_power_off)
		cmd = LINUX_REBOOT_CMD_HALT;

	mutex_lock(&reboot_mutex);
	switch (cmd) {
	case LINUX_REBOOT_CMD_RESTART:
		kernel_restart(NULL);
		break;

	case LINUX_REBOOT_CMD_CAD_ON:
		C_A_D = 1;
		break;

	case LINUX_REBOOT_CMD_CAD_OFF:
		C_A_D = 0;
		break;

	case LINUX_REBOOT_CMD_HALT:
		kernel_halt();
		do_exit(0);
		panic("cannot halt");

	case LINUX_REBOOT_CMD_POWER_OFF:
		kernel_power_off();
		do_exit(0);
		break;

	case LINUX_REBOOT_CMD_RESTART2:
		ret = strncpy_from_user(&buffer[0], arg, sizeof(buffer) - 1);
		if (ret < 0) {
			ret = -EFAULT;
			break;
		}
		buffer[sizeof(buffer) - 1] = '\0';

		kernel_restart(buffer);
		break;

#ifdef CONFIG_KEXEC_CORE
	case LINUX_REBOOT_CMD_KEXEC:
		ret = kernel_kexec();
		break;
#endif

#ifdef CONFIG_HIBERNATION
	case LINUX_REBOOT_CMD_SW_SUSPEND:
		ret = hibernate();
		break;
#endif

	default:
		ret = -EINVAL;
		break;
	}
	mutex_unlock(&reboot_mutex);
	return ret;
}

static void deferred_cad(struct work_struct *dummy)
{
	kernel_restart(NULL);
}

/*
 * This function gets called by ctrl-alt-del - ie the keyboard interrupt.
 * As it's called within an interrupt, it may NOT sync: the only choice
 * is whether to reboot at once, or just ignore the ctrl-alt-del.
 */
void ctrl_alt_del(void)
{
	static DECLARE_WORK(cad_work, deferred_cad);

	if (C_A_D)
		schedule_work(&cad_work);
	else
		kill_cad_pid(SIGINT, 1);
}

char poweroff_cmd[POWEROFF_CMD_PATH_LEN] = "/sbin/poweroff";
static const char reboot_cmd[] = "/sbin/reboot";

static int run_cmd(const char *cmd)
{
	char **argv;
	static char *envp[] = {
		"HOME=/",
		"PATH=/sbin:/bin:/usr/sbin:/usr/bin",
		NULL
	};
	int ret;
	argv = argv_split(GFP_KERNEL, cmd, NULL);
	if (argv) {
		ret = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
		argv_free(argv);
	} else {
		ret = -ENOMEM;
	}

	return ret;
}

static int __orderly_reboot(void)
{
	int ret;

	ret = run_cmd(reboot_cmd);

	if (ret) {
		pr_warn("Failed to start orderly reboot: forcing the issue\n");
		emergency_sync();
		kernel_restart(NULL);
	}

	return ret;
}

static int __orderly_poweroff(bool force)
{
	int ret;

	ret = run_cmd(poweroff_cmd);

	if (ret && force) {
		pr_warn("Failed to start orderly shutdown: forcing the issue\n");

		/*
		 * I guess this should try to kick off some daemon to sync and
		 * poweroff asap.  Or not even bother syncing if we're doing an
		 * emergency shutdown?
		 */
		emergency_sync();
		kernel_power_off();
	}

	return ret;
}

static bool poweroff_force;

static void poweroff_work_func(struct work_struct *work)
{
	__orderly_poweroff(poweroff_force);
}

static DECLARE_WORK(poweroff_work, poweroff_work_func);

/**
 * orderly_poweroff - Trigger an orderly system poweroff
 * @force: force poweroff if command execution fails
 *
 * This may be called from any context to trigger a system shutdown.
 * If the orderly shutdown fails, it will force an immediate shutdown.
 */
void orderly_poweroff(bool force)
{
	if (force) /* do not override the pending "true" */
		poweroff_force = true;
	schedule_work(&poweroff_work);
}
EXPORT_SYMBOL_GPL(orderly_poweroff);

static void reboot_work_func(struct work_struct *work)
{
	__orderly_reboot();
}

static DECLARE_WORK(reboot_work, reboot_work_func);

/**
 * orderly_reboot - Trigger an orderly system reboot
 *
 * This may be called from any context to trigger a system reboot.
 * If the orderly reboot fails, it will force an immediate reboot.
 */
void orderly_reboot(void)
{
	schedule_work(&reboot_work);
}
EXPORT_SYMBOL_GPL(orderly_reboot);

static int __init reboot_setup(char *str)
{
	for (;;) {
		/*
		 * Having anything passed on the command line via
		 * reboot= will cause us to disable DMI checking
		 * below.
		 */
		reboot_default = 0;

		switch (*str) {
		case 'w':
			reboot_mode = REBOOT_WARM;
			break;

		case 'c':
			reboot_mode = REBOOT_COLD;
			break;

		case 'h':
			reboot_mode = REBOOT_HARD;
			break;

		case 's':
			if (isdigit(*(str+1)))
				reboot_cpu = simple_strtoul(str+1, NULL, 0);
			else if (str[1] == 'm' && str[2] == 'p' &&
							isdigit(*(str+3)))
				reboot_cpu = simple_strtoul(str+3, NULL, 0);
			else
				reboot_mode = REBOOT_SOFT;
			break;

		case 'g':
			reboot_mode = REBOOT_GPIO;
			break;

		case 'b':
		case 'a':
		case 'k':
		case 't':
		case 'e':
		case 'p':
			reboot_type = *str;
			break;

		case 'f':
			reboot_force = 1;
			break;
		}

		str = strchr(str, ',');
		if (str)
			str++;
		else
			break;
	}
	return 1;
}
__setup("reboot=", reboot_setup);
