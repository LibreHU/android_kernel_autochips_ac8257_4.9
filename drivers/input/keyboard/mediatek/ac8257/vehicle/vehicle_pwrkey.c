/*
 * Copyright (C) 2019 Autochips Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/of_platform.h>
#include <linux/module.h>
#include <linux/reboot.h>
#include <linux/compiler.h>
#include <linux/of_address.h>
#include <asm/io.h>
#include <linux/inet.h>
#include <linux/socket.h>
#include <linux/net.h>
#include <net/sock.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>

enum pwrkey_socket_status {
    SOCKET_DISCONNECT,
    SOCKET_CONNECTED,
};
/* default value, referenced by /hardware/interfaces/automotive/vehicle/2.0/default/impl/vhal_v2_0/SocketComm.cpp */
#define MS_LONG_PRESS 2000
static const int g_vhal_port = 33452;
static enum pwrkey_socket_status socket_status = SOCKET_DISCONNECT;

/* kernel-4.9 socket */
struct socket *g_vhal_sock = NULL;
struct socket *client_sock = NULL;

static struct socket* pwrkey_vhal_open(void)
{
    int ret = 0;

    if (g_vhal_sock)
        return g_vhal_sock;

    ret = sock_create_kern(&init_net, AF_INET, SOCK_STREAM, 0, &g_vhal_sock);
    if (ret < 0) {
        pr_err("[PWRKEY] sock_create_kern failed, ret=%d.\n", ret);
        return NULL;
    }

    return g_vhal_sock;
}

static void pwrkey_vhal_close(void)
{
    if (g_vhal_sock != NULL) {
        kernel_sock_shutdown(g_vhal_sock, SHUT_RDWR);
        sock_release(g_vhal_sock);
    }

    socket_status = SOCKET_DISCONNECT;
    g_vhal_sock = NULL;
}


static int pwrkey_vhal_connect(int retry)
{
    struct sockaddr_in server_addr;
    int fd = 0;
    int ret = -1;

    if (SOCKET_CONNECTED == socket_status)
        return 0;

    /* check sock status */
    if (!g_vhal_sock && !pwrkey_vhal_open()) {
        pr_err("[PWRKEY] connect vhal failed when create socket.\n");
        return fd;
    }

    /* set server */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(g_vhal_port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    while (retry--) {
        /* init socket for vehicle HAL */
        ret = kernel_connect(g_vhal_sock, (struct sockaddr*)&server_addr, sizeof(server_addr), 0);
        if (ret < 0)
            pr_err("[PWRKEY] connect to vhal failed, remain %d time, ret:%d.\n", retry, ret);
        else
            break;
    }

    if (ret < 0) {
        /* release sock */
        pwrkey_vhal_close();
        return ret;
    }

    socket_status = SOCKET_CONNECTED;

    return ret;
}

static int pwrkey_vhal_send(void *msg, int size)
{
    struct msghdr kmsg;
    struct kvec iov[2];
    int msg_len = htonl(size);
    int len = 0;

    if (SOCKET_CONNECTED != socket_status) {
        pr_err("[PWRKEY] send not accept.\n");
        return -1;
    }

    memset(&kmsg, 0, sizeof(kmsg));
    /* iov[0] is an interge which means the len of iov 2, it is just to fit the format from SocketComm.cpp*/
    iov[0].iov_base = &msg_len;
    iov[0].iov_len = sizeof(msg_len);
    iov[1].iov_base = msg;
    iov[1].iov_len = size;

    /* set only 1 kvec */
    len = kernel_sendmsg(g_vhal_sock, &kmsg, iov, 2, size + sizeof(msg_len));
    if (len < 0) {
        pr_err("[PWRKEY] kernel_sendmsg failed.\n");
        return len;
    }
    pr_err("[PWRKEY] kernel_sendmsg len=%d.\n", len);

    return 0;
}

static int pwrkey_vhal_recv(void *msg, int size)
{
    /* kernel donot need recv msg */
    return 0;
}

static long unsigned int g_pwrkey_current_time = 0;
static void shutdown_work(void)
{
    if (pwrkey_vhal_connect(3))
        return ;

    if (pwrkey_vhal_send("shutdown", 8) < 0) {
        pr_err("[PWRKEY] shutdown_work send msg fail.\n");
    }
}

static void quickboot_work(void)
{
    if (pwrkey_vhal_connect(3))
        return ;

    if (pwrkey_vhal_send("suspend", 8) < 0) {
        pr_err("[PWRKEY] qb_work send msg fail.\n");
    }
}

int pwrkey_check_mode(void)
{
    if (!g_pwrkey_current_time) {
        /* get current time, when key push down */
        g_pwrkey_current_time = jiffies;
        pr_info("[PWRKEY] first push down at %lu, 2000ms=%lujiffies \r\n", g_pwrkey_current_time, msecs_to_jiffies(MS_LONG_PRESS));
    } else {
        if ((jiffies - g_pwrkey_current_time) >= msecs_to_jiffies(MS_LONG_PRESS)) {
            // Shutdown
            pr_info("[PWRKEY] second push down at %lu, going to shutdown \r\n", jiffies);
            shutdown_work();
        } else {
            // QuickBoot
            pr_info("[PWRKEY] second push down at %lu, going to sleep \r\n", jiffies);
            quickboot_work();
        }
        g_pwrkey_current_time = 0;
    }

    return 0;
}
EXPORT_SYMBOL(pwrkey_check_mode);

void pwrkey_socket_test(void)
{
    char *msg = "shutdown";

    if (NULL == pwrkey_vhal_open()) {
        pr_err("[PWRKEY TEST] open failed.\n");
        return ;
    }

    if (!pwrkey_vhal_connect(3)) {
        pr_err("[PWRKEY TEST] connect failed.\n");
        return ;
    }

    if (!pwrkey_vhal_send(&msg, 4)) {
        pr_err("[PWRKEY TEST] send failed.\n");
        return ;
    }

    if (!pwrkey_vhal_recv(&msg, 4)) {
        pr_err("[PWRKEY TEST] recv failed.\n");
        return ;
    }

    pwrkey_vhal_close();
}

#ifdef CONFIG_PWRKEY_TEST
static int pwrkey_open(struct inode* inode, struct file *file)
{
    int ret = 0;
    if (NULL == pwrkey_vhal_open()) {
        pr_err("[PWRKEY open] open failed.\n");
        return -1;
    }

    ret = pwrkey_vhal_connect(3);
    if (ret < 0) {
        pr_err("[PWRKEY open] connect failed.\n");
        return -1;
    }

    return 0;
}

static ssize_t pwrkey_read(struct file *file, char __user *in, size_t size, loff_t *off)
{
    int msg = size;

    if (0 == pwrkey_vhal_recv(&msg, 4)) {
        pr_err("[PWRKEY read] recv not support.\n");
        return -1;
    }

    if (0 > pwrkey_vhal_recv(&msg, 4)) {
        pr_err("[PWRKEY read] recv failed.\n");
        return -1;
    }

    if (copy_to_user((void *)in, (void *)&msg, 4)) {
        pr_err("[PWRKEY read] copy_to_user failed.\n");
        msg = -EFAULT;
    }

    return msg;
}

static ssize_t pwrkey_write(struct file *file, const char __user *in, size_t size, loff_t *off)
{
    if (pwrkey_vhal_send("shutdown", 8) < 0) {
        pr_err("[PWRKEY write] send failed.\n");
        return -EFAULT;
    }

    return size;
}

struct file_operations pwrkey_fops = {
    .owner = THIS_MODULE,
    .open = pwrkey_open,
    .read = pwrkey_read,
    .write = pwrkey_write,
};

static struct miscdevice pwrkey_misc_dev = {
    MISC_DYNAMIC_MINOR,
    "pwrkey-dev",
    &pwrkey_fops,
};
#endif

static int pwrkey_probe(struct platform_device *pdev)
{
#ifdef CONFIG_PWRKEY_TEST
    int ret = 0;

    ret = misc_register(&pwrkey_misc_dev);
    if (ret) {
        pr_err("[PWRKEY]: Unable to register \"pwrkey\" misc device\n");
        return ret;
    }
#endif

    g_pwrkey_current_time = 0;

    pr_info("[PWRKEY] restart_poweroff_probe init\n");
    return 0;
}

static int pwrkey_remove(struct platform_device *pdev)
{
    pr_info("[PWRKEY] restart_poweroff_remove end\n");

    return 0;
}

static const struct of_device_id of_pwrkey_match[] = {
    { .compatible = "autochips,powerkey", },
    {},
};

static struct platform_driver pwrkey_driver = {
    .probe = pwrkey_probe,
    .remove = pwrkey_remove,
    .driver = {
        .name = "atc-pwrkey",
        .of_match_table = of_pwrkey_match,
    },
};

static int __init pwrkey_drv_init(void)
{
    int ret = 0;

    ret = platform_driver_register(&pwrkey_driver);
    if (ret) {
        pr_err("[PWRKEY] pwrkey driver init fail.\n");
        return ret;
    }

    return 0;
}

static void __exit pwrkey_drv_exit(void)
{
    platform_driver_unregister(&pwrkey_driver);
}

module_init(pwrkey_drv_init);
module_exit(pwrkey_drv_exit);

MODULE_DESCRIPTION("AC8257 Vehicle PowerKey Driver");
MODULE_LICENSE("GPL v2");
