/*
* Copyright (c) 2016 AutoChips Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/



#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/wait.h>
#include "x_os.h"
#include <linux/spinlock_types.h>
#include <linux/types.h>


static DEFINE_SPINLOCK(ac83xx_msg_q_lock);

#define MSGQ_NAME_LEN           16
#define MSGQ_MAX_DATA_SIZE      4095
#define MSGQ_MAX_MSGS           4095

typedef struct os_msg_q_light
{
    struct os_msg_q_light *previous;
    struct os_msg_q_light *next;
    wait_queue_head_t read_wq;
    wait_queue_head_t write_wq;
    u8 *read;
    u8 *write;
    u8 *end;
    u32 message_size;
    u32 z_maxsize;
    u16 ui2_msg_count;
    s16 i2_refcount;
    char s_name[MSGQ_NAME_LEN + 1];
    u8 start[1];
} OS_MSGQ_LIGHT_T;


static OS_MSGQ_LIGHT_T *s_msg_q_list;


static void msg_q_list_add(OS_MSGQ_LIGHT_T *pt_msg_q)
{
    if (s_msg_q_list != NULL)
    {
        pt_msg_q->previous = s_msg_q_list->previous;
        pt_msg_q->next = s_msg_q_list;
        s_msg_q_list->previous->next = pt_msg_q;
        s_msg_q_list->previous = pt_msg_q;
    }
    else
    {
        s_msg_q_list = pt_msg_q->next = pt_msg_q->previous = pt_msg_q;
    }
}


static void msg_q_list_remove(OS_MSGQ_LIGHT_T *pt_msg_q)
{
    if (pt_msg_q->previous == pt_msg_q)
    {
        s_msg_q_list = NULL;
    }
    else
    {
        pt_msg_q->previous->next = pt_msg_q->next;
        pt_msg_q->next->previous = pt_msg_q->previous;
        if (s_msg_q_list == pt_msg_q)
        {
            s_msg_q_list = pt_msg_q->next;
        }
    }
}


static OS_MSGQ_LIGHT_T *msg_q_find_obj(const char *ps_name)
{
    OS_MSGQ_LIGHT_T *pt_msg_q = s_msg_q_list;
    if (pt_msg_q == NULL)
    {
        return NULL;
    }
    do
    {
        if (strncmp(pt_msg_q->s_name, ps_name, (size_t)MSGQ_NAME_LEN) == 0)
        {
            return pt_msg_q;
        }
        pt_msg_q = pt_msg_q->next;
    } while (pt_msg_q != s_msg_q_list);

    return NULL;
}


__s32 x_msg_q_create (uintptr_t*    ph_msg_hdl,
                             const char*  ps_name,
                             size_t       z_msg_size,
                             __u16       ui2_msg_count)
{
    OS_MSGQ_LIGHT_T *pt_msg_q;
    __u32 message_size;
    unsigned long flags;

    /* check arguments */
    if ((ps_name == NULL) || (ps_name[0] == '\0') || (ph_msg_hdl == NULL) ||
        (z_msg_size > (__u32)MSGQ_MAX_DATA_SIZE) || (ui2_msg_count > (__u32)MSGQ_MAX_MSGS) ||
        (z_msg_size == (__u32)0) || (ui2_msg_count == (__u32)0))
    {
    	 printk("[OSAL] Invalid arg from x_msg_q_create\n");
        return OSR_INV_ARG;
    }

    message_size = (__u32)sizeof(__u32) + ((z_msg_size + (__u32)3) & ~((__u32)3));
    pt_msg_q = kcalloc((size_t)1, (sizeof(OS_MSGQ_LIGHT_T) - sizeof(__u8) + (size_t)(message_size * ((__u32)ui2_msg_count + (__u32)1))), GFP_KERNEL);
    if (pt_msg_q == NULL)
    {
    	 printk("[OSAL] No resource from x_msg_q_create\n");
        return OSR_NO_RESOURCE;
    }
    //FILL_CALLER(pt_msg_q);

    pt_msg_q->read = pt_msg_q->start;
    pt_msg_q->write = pt_msg_q->start;
    pt_msg_q->end = pt_msg_q->start + message_size * (ui2_msg_count + 1);
    pt_msg_q->message_size = message_size;
    pt_msg_q->z_maxsize = z_msg_size;
    pt_msg_q->ui2_msg_count = ui2_msg_count;
    pt_msg_q->i2_refcount = 1;
    strncpy(pt_msg_q->s_name, ps_name, (size_t)MSGQ_NAME_LEN);

    init_waitqueue_head(&pt_msg_q->read_wq);
    init_waitqueue_head(&pt_msg_q->write_wq);

    //local_irq_save(flags);
    spin_lock_irqsave(&ac83xx_msg_q_lock, flags);
    if (msg_q_find_obj(ps_name) != NULL)
    {
        //local_irq_restore(flags);
        spin_unlock_irqrestore(&ac83xx_msg_q_lock, flags);
        kfree(pt_msg_q);
	 printk("[OSAL] Already exist from x_msg_q_create\n");
        return OSR_EXIST;
    }
    msg_q_list_add(pt_msg_q);
    //local_irq_restore(flags);
    spin_unlock_irqrestore(&ac83xx_msg_q_lock, flags);

    *ph_msg_hdl = (u32)(pt_msg_q);
    return OSR_OK;
}

EXPORT_SYMBOL(x_msg_q_create);


__s32
x_msg_q_attach(uintptr_t     *ph_msg_hdl,
               const char   *ps_name)
{
    OS_MSGQ_LIGHT_T *pt_msg_q;
    unsigned long flags;

    /* arguments check */
    if ((ps_name == NULL) || (ps_name[0] == '\0') ||
        (ph_msg_hdl == NULL))
    {
    	 printk("[OSAL] Invalid arg from x_msg_q_attach\n");
        return OSR_INV_ARG;
    }

    //local_irq_save(flags);
    spin_lock_irqsave(&ac83xx_msg_q_lock, flags);
    pt_msg_q = msg_q_find_obj(ps_name);
    if (pt_msg_q == NULL)
    {
        //local_irq_restore(flags);
        spin_unlock_irqrestore(&ac83xx_msg_q_lock, flags);
	 printk("[drvwin32]MsgQ not exist from x_msg_q_attach\n");
        return OSR_NOT_EXIST;
    }

    pt_msg_q->i2_refcount++;
    //local_irq_restore(flags);
    spin_unlock_irqrestore(&ac83xx_msg_q_lock, flags);

    *ph_msg_hdl = (u32)(pt_msg_q);

    return OSR_OK;
}

EXPORT_SYMBOL(x_msg_q_attach);


__s32
x_msg_q_delete(uintptr_t h_msg_hdl)
{
    OS_MSGQ_LIGHT_T *pt_msg_q;
    __s16 i2_refcount;
    unsigned long flags;

    pt_msg_q = (OS_MSGQ_LIGHT_T *)(h_msg_hdl);

    //local_irq_save(flags);
    spin_lock_irqsave(&ac83xx_msg_q_lock, flags);
	--pt_msg_q->i2_refcount;
    i2_refcount = pt_msg_q->i2_refcount;
    if (i2_refcount > 0)
    {
        //local_irq_restore(flags);
        spin_unlock_irqrestore(&ac83xx_msg_q_lock, flags);
        return OSR_OK;
    }
    msg_q_list_remove(pt_msg_q);
    //local_irq_restore(flags);
    spin_unlock_irqrestore(&ac83xx_msg_q_lock, flags);

    kfree(pt_msg_q);
    return OSR_OK;
}

EXPORT_SYMBOL(x_msg_q_delete);


__s32 x_msg_q_send (uintptr_t	  h_msg_hdl,
				   const void*	pv_msg,
				   size_t		z_size,
				   __u8 	   ui1_priority)

{
    OS_MSGQ_LIGHT_T *pt_msg_q;
    __u8 *write;
    unsigned long flags;

    if ((pv_msg == NULL) || (z_size == 0))
    {
     	 printk("[OSAL] Invalid arg from x_msg_q_send\n");
        return OSR_INV_ARG;
    }

    pt_msg_q = (OS_MSGQ_LIGHT_T *)(h_msg_hdl);

    if (z_size > pt_msg_q->z_maxsize)
    {
    	printk("[OSAL] Msg size too big from x_msg_q_send\n");
        return OSR_TOO_BIG;
    }

    //local_irq_save(flags);
    spin_lock_irqsave(&ac83xx_msg_q_lock, flags);
    write = pt_msg_q->write + pt_msg_q->message_size;
    if (write == pt_msg_q->end)
    {
        write = pt_msg_q->start;
    }
    if (write == pt_msg_q->read)
    {
        //local_irq_restore(flags);
        spin_unlock_irqrestore(&ac83xx_msg_q_lock, flags);
	 printk("[OSAL] Msg too many from x_msg_q_send\n");
        return OSR_TOO_MANY;
    }
    *(__u32 *)(pt_msg_q->write) = z_size;
    memcpy(pt_msg_q->write + 4, pv_msg, z_size);
    pt_msg_q->write = write;
    wake_up_all(&(pt_msg_q->write_wq));
    //local_irq_restore(flags);
    spin_unlock_irqrestore(&ac83xx_msg_q_lock, flags);

    return OSR_OK;
}

EXPORT_SYMBOL(x_msg_q_send);


__s32
x_msg_q_receive(__u16*        pui2_index,
                      void*          pv_msg,
                      size_t*        pz_size,
                      uintptr_t*      ph_msg_q_mon_list,
                      __u16         ui2_msg_q_mon_count,
                      MSGQ_OPTION_T  e_options)
{
    OS_MSGQ_LIGHT_T *pt_msg_q;
    __u8 *read;
    __u32 z_size;
	unsigned long flags;

    /* check arguments */
    if ((e_options != X_MSGQ_OPTION_WAIT) && (e_options != X_MSGQ_OPTION_NOWAIT))
    {
    	 printk("[OSAL] Invalid arg from x_msg_q_receive, e_option wrong\n");
        return OSR_INV_ARG;
    }

    if ((pui2_index == NULL) || (pv_msg == NULL) ||
        (pz_size == NULL) || (*pz_size == 0) ||
        (ph_msg_q_mon_list == NULL) || (ui2_msg_q_mon_count == 0))
    {
    	 printk("[OSAL] Invalid arg from x_msg_q_receive\n");
        return OSR_INV_ARG;
    }

    if (ui2_msg_q_mon_count != (__u16)1)
    {
    	 printk("[drvwin32]Not support from x_msg_q_receive\n");
        return OSR_NOT_SUPPORT;
    }

    pt_msg_q = (OS_MSGQ_LIGHT_T *)(ph_msg_q_mon_list[0]);
    if (pt_msg_q->read == pt_msg_q->write)
    {
        if (e_options == X_MSGQ_OPTION_NOWAIT)
        {
        	printk("[OSAL] No msg from x_msg_q_receive\n");
            return OSR_NO_MSG;
        }
        wait_event(pt_msg_q->write_wq, (pt_msg_q->read != pt_msg_q->write));
    }
	spin_lock_irqsave(&ac83xx_msg_q_lock, flags);
    read = pt_msg_q->read;
    z_size = *(__u32 *)(read);
    if (z_size > *pz_size)
    {
        z_size = *pz_size;
    }
    memcpy(pv_msg, read + 4, z_size);
    read += pt_msg_q->message_size;
    if (read == pt_msg_q->end)
    {
        read = pt_msg_q->start;
    }
    pt_msg_q->read = read;
    wake_up_all(&(pt_msg_q->read_wq));

    *pz_size = z_size;
	spin_unlock_irqrestore(&ac83xx_msg_q_lock, flags);

    return OSR_OK;
}

EXPORT_SYMBOL(x_msg_q_receive);


__s32
x_msg_q_receive_timeout(__u16          *pui2_index,
                        void            *pv_msg,
                        size_t          *pz_size,
                        uintptr_t        *ph_msgq_hdl_list,
                        __u16          ui2_msgq_hdl_count,
                        __u32          ui4_time)
{
    static const __s32 quantum_ms = (__s32)((__s32)1000 / (__s32)HZ);
    OS_MSGQ_LIGHT_T *pt_msg_q;
    __u8 *read;
    __u32 z_size;
    __s32 ret;
	unsigned long flags;

    if ((pui2_index == NULL) || (pv_msg == NULL) ||
        (pz_size == NULL) || (*pz_size == 0) ||
        (ph_msgq_hdl_list == NULL) || (ui2_msgq_hdl_count == 0))
    {
    	 printk("[OSAL] Invalid arg from x_msg_q_receive_timeout\n");
        return OSR_INV_ARG;
    }

    if (ui2_msgq_hdl_count != (__u16)1)
    {
    	 printk("[OSAL] Not support from x_msg_q_receive_timeout\n");
        return OSR_NOT_SUPPORT;
    }

    pt_msg_q = (OS_MSGQ_LIGHT_T *)(ph_msgq_hdl_list[0]);
    if (pt_msg_q->read == pt_msg_q->write)
    {
        ret = wait_event_timeout(pt_msg_q->write_wq, (pt_msg_q->read != pt_msg_q->write), (long)(ui4_time / (__u32)quantum_ms));
        if (ret == 0)
        {
            return OSR_TIMEOUT;
        }
    }
	spin_lock_irqsave(&ac83xx_msg_q_lock, flags);
    read = pt_msg_q->read;
    z_size = *(__u32 *)(read);
    if (z_size > *pz_size)
    {
        z_size = *pz_size;
    }
    memcpy(pv_msg, read + 4, z_size);
    read += pt_msg_q->message_size;
    if (read == pt_msg_q->end)
    {
        read = pt_msg_q->start;
    }
    pt_msg_q->read = read;
    wake_up_all(&(pt_msg_q->read_wq));

    *pz_size = z_size;
	spin_unlock_irqrestore(&ac83xx_msg_q_lock, flags);
    return OSR_OK;
}

EXPORT_SYMBOL(x_msg_q_receive_timeout);


__s32
x_msg_q_num_msgs(uintptr_t   h_msg_hdl,
                 __u16     *pui2_num_msgs)
{
    OS_MSGQ_LIGHT_T *pt_msg_q;
    __u32 messages;

    if (pui2_num_msgs == NULL)
    {
    	printk("[OSAL] Invalid arg from x_msg_q_num_msgs\n");
        return OSR_INV_ARG;
    }

    pt_msg_q = (OS_MSGQ_LIGHT_T *)(h_msg_hdl);

    messages = pt_msg_q->write - pt_msg_q->read;
    if (pt_msg_q->write < pt_msg_q->read)
    {
        messages += pt_msg_q->end - pt_msg_q->start;
    }
    messages /= pt_msg_q->message_size;
    *pui2_num_msgs = (__u16)(messages);

    return OSR_OK;
}

EXPORT_SYMBOL(x_msg_q_num_msgs);


__s32 x_msg_q_get_max_msg_size (uintptr_t  h_msg_hdl,
                                size_t*   pz_maxsize)                         
{
    OS_MSGQ_LIGHT_T *pt_msg_q;

    if (pz_maxsize == NULL)
    {
    	printk("[OSAL] Invalid arg from x_msg_q_get_max_msg_size\n");
        return OSR_INV_ARG;
    }

    pt_msg_q = (OS_MSGQ_LIGHT_T *)(h_msg_hdl);
    *pz_maxsize = pt_msg_q->z_maxsize;

    return OSR_OK;
}

EXPORT_SYMBOL(x_msg_q_get_max_msg_size);


__s32
msg_q_init(void)
{
    return OSR_OK;
}

