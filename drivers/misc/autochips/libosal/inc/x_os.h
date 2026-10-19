#ifndef _X_OS_H
#define _X_OS_H

#ifdef __linux__
#ifndef __ARM2__
#ifdef __KERNEL__
#include <linux/mm.h>
#endif
#endif
#endif

#ifdef __ARM2__
typedef __u32 uintptr_t;
#endif

//#include <windows.h>
//#include <types.h>
//#include <memory.h>
#ifndef __KERNEL__
#ifndef __ARM2__
#pragma message "compile in user mode"
#include <sys/types.h>     //For android project
#include <linux/types.h>   //For linux project
#include <stdint.h>
#include <stdbool.h>
#endif
#else
//#pragma message "compile in kernel mode"
#ifndef __ARM2__
#include <linux/types.h>   //For linux project
#include "x_typedef.h"
#else
#include "x_types.h"
#endif
#endif

#include "u_os.h"
//#include <x_assert.h>
//#include "memdbg_c.h"

#if 1
void x_mem_free(void *pv_mem_block);
void * x_mem_alloc(size_t u4Size);
void* x_mem_alloc_ret_phy_addr (size_t  z_size, __u32 *pu4PhyAddr );
void  x_mem_free_ret_phy_addr (void*  pv_mem_block );
void* x_mem_aligned_alloc(size_t z_size,__u32 u4Align);
#else
//#include "memdbg_c.h"

//#define x_mem_alloc(x)    MemDbgAlloc((x), __FILE__, __LINE__)
//#define x_mem_free(x)    MemDbgFree((x), __FILE__, __LINE__)
#endif
void * x_mem_ch2_alloc(size_t u4Size);

//#define x_thread_delay  Sleep

#define x_memcpy  memcpy

#define  x_memcmp memcmp
#define x_memset memset
#define x_memchr memchr

#ifndef __linux__
#define x_strcpy strcpy
#else
extern char *x_strcpy(char *ps_to, const char *ps_from);
#endif

#define x_strncpy strncpy

#define x_strcmp strcmp

#define x_strncmp strncmp


#ifndef __linux__
#define x_strcat strcat
#endif

#define x_strncat strncat

#define x_strchr strchr

#define x_strrchr strrchr

#ifndef __linux__
#define x_strstr strstr
#endif
#define x_strlen strlen

#define x_strspn strspn
#define x_strcspn strcspn
extern __u64 x_strtoull (const char*  pc_beg_ptr,
                          char**       ppc_end_ptr,
                          __u8        ui1_base);

extern __s64 x_strtoll (const char*  pc_beg_ptr,
                        char**       ppc_end_ptr,
                        __u8        ui1_base);

extern char* x_str_toupper (char*  ps_str);

extern char* x_str_tolower (char*  ps_str);

#define x_cli_parser_arg


/* Thread API's */
/*typedef void (*x_os_thread_main_fct) (void*  pv_arg);

typedef void (*x_os_thread_pvt_del_fct) (__u32  ui4_key,
                                         void*   pv_pvt);*/

extern __s32 x_thread_create (uintptr_t             *ph_th_hdl,
                              const char*           ps_name,
                              size_t                z_stack_size,
                              __u8                 ui1_priority,
                              x_os_thread_main_fct  pf_main_rtn,
                              size_t                z_arg_size,
                              void*                 pv_arg);

extern void x_thread_exit (void);

extern void x_thread_delay (__u32  u4Delay);

extern __s32 x_thread_set_pri (uintptr_t  h_th_hdl,
                               __u8     ui1_new_pri);

extern __s32 x_thread_get_name  (uintptr_t  h_th_hdl, __u32* s_name);

extern __s32 x_thread_get_pri (uintptr_t  h_th_hdl,
                               __u8*    pui1_pri);

extern void x_thread_suspend (void);

extern __s32 x_thread_resume (uintptr_t  h_th_hdl);

extern __s32 x_thread_self (uintptr_t *  ph_th_hdl);

extern __s32 x_thread_stack_stats (uintptr_t  h_th_hdl,
                                   size_t*   pz_alloc_stack,
                                   size_t*   pz_max_used_stack);

extern __s32 x_thread_set_pvt (__u32                   ui4_key,
                               x_os_thread_pvt_del_fct  pf_pvt_del,
                               void*                    pv_pvt);
extern __s32 x_thread_get_pvt (__u32  ui4_key,
                               void**  ppv_pvt);
extern __s32 x_thread_del_pvt (__u32  ui4_key);

//#if defined(CONFIG_ATC_OS_android) && defined(CONFIG_ATC_PLATFORM_ac823x)
//extern uintptr_t x_thread_find_obj(const char *ps_name);
//#else
extern uintptr_t x_thread_find_obj(const char *ps_name);
//#endif
/* ISR API's */
//typedef void (*x_os_isr_fct) (__u16  ui2_vector_id);

extern __s32 x_reg_isr_ex (__u16         ui2_vec_id,
                           x_os_isr_fct   pf_isr,
                           x_os_isr_fct*  ppf_old_isr,
                           ISR_FLAG_T     e_flags);

extern __s32 x_reg_isr (__u16         ui2_vec_id,
                        x_os_isr_fct   pf_isr,
                        x_os_isr_fct*  ppf_old_isr);


/* Semaphhore API's */
extern __s32 x_sema_force_delete (uintptr_t  h_sema_hdl);
extern __s32 x_sema_lock_timeout (uintptr_t  h_sema_hdl,
                                  __u32    ui4_time);
extern __s32 x_sema_create (uintptr_t    *ph_sema_hdl,
                            SEMA_TYPE_T  e_types,
                            __u32       ui4_init_value);

extern __s32 x_sema_delete (uintptr_t  h_sema_hdl);
extern __s32 x_sema_lock (uintptr_t       h_sema_hdl,SEMA_OPTION_T e_options);
extern __s32 x_sema_unlock (uintptr_t  h_sema_hdl);

/* Timer API's */
//typedef void (*x_os_timer_cb_fct) (__u32  pt_tm_handle, void*     pv_tag);

extern __s32 x_timer_create (uintptr_t  ph_timer);

extern __s32 x_timer_start (uintptr_t           h_timer,
                            __u32             ui4_delay,
                            TIMER_FLAG_T       e_flags,
                            x_os_timer_cb_fct  pf_callback,
                            void*              pv_tag);

extern __s32 x_timer_stop (uintptr_t  h_timer);

extern __s32 x_timer_delete (uintptr_t  h_timer);

extern __s32 x_timer_resume (uintptr_t  h_timer);

extern __u32 x_os_get_sys_tick (void);

extern __u32 x_os_get_sys_tick_period (void);

//Queue API
__s32 x_queue_create(uintptr_t  phQueue, __u32 u4MaxSize);
__s32 x_queue_delete(uintptr_t hQueue);
__s32 x_queue_pop_head(uintptr_t hQueue, void **ppData);
__s32 x_queue_push_tail(uintptr_t  hQueue, void *pData);
__s32 x_queue_peek_nth(uintptr_t hQueue, __u32 u4Index, void **ppData);
__s32 x_queue_get_length(uintptr_t  hQueue, __u32 *pLength);

// MemChunk API
__s32 x_mem_chunk_create(uintptr_t phMemChunk,
								__u32	u4MemSize,
								__u32	u4ChunkSize);
__s32 x_mem_chunk_delete(uintptr_t hMemChunk);
__s32 x_mem_chunk_alloc(uintptr_t hMemChunk, void **pptr);
__s32 x_mem_chunk_free(uintptr_t hMemChunk, void *ptr);

void *x_map_physical_address(
    __u32 dwPhysAddress,
    __u32 dwSize,
    bool  bCacheEnable
    );


__u32 x_crit_start(void);
void x_crit_end(__u32 t_old_level);

void* x_event_create(void * lpEventAttributes, bool bManualReset, bool bInitialState, const char * lpName);

unsigned long x_event_wait_for_objects(unsigned long nCount, const void**  lpHandles, bool bWaitAll, unsigned long dwMilliseconds);

void* x_event_open(unsigned long dwDesiredAccess, bool bInheritHandle, const char * lpName);

bool x_event_destroy(void* hEvent);

bool x_event_set(void* hEvent);

bool x_event_set_data(void* hEvent, unsigned long dwData);

unsigned long x_event_get_data(void* hEvent);

bool x_event_reset(void* hEvent);

/* Event_group API's */
extern __s32 x_ev_group_create (uintptr_t        *ph_hdl,
                                const char       *ps_name,
                                EV_GRP_EVENT_T   e_init_events);

extern __s32 x_ev_group_attach (uintptr_t     *ph_hdl,
                                const char   *ps_name);

extern __s32 x_ev_group_delete (uintptr_t  h_hdl);

extern __s32 x_ev_group_set_event (uintptr_t             h_hdl,
                                   EV_GRP_EVENT_T       e_events,
                                   EV_GRP_OPERATION_T   e_op);

extern __s32 x_ev_group_wait_event (uintptr_t            h_hdl,
                                    EV_GRP_EVENT_T      e_events_req,
                                    EV_GRP_EVENT_T      *pe_events_got,
                                    EV_GRP_OPERATION_T  e_op);

extern __s32 x_ev_group_wait_event_timeout(uintptr_t           h_hdl,
                                           EV_GRP_EVENT_T     e_events_req,
                                           EV_GRP_EVENT_T     *pe_events_got,
                                           EV_GRP_OPERATION_T e_op,
                                           __u32             ui4_time);

extern __s32 x_ev_group_get_info (uintptr_t          h_hdl,
                                  EV_GRP_EVENT_T    *pe_cur_events,
                                  __u8             *pui1_num_thread_waiting,
                                  char              *ps_ev_group_name,
                                  char              *ps_first_wait_thread);


/* MsgQ API's */
extern __s32 x_msg_q_create (uintptr_t *   ph_msg_hdl,
                             const char*  ps_name,
                             size_t       z_msg_size,
                             __u16       ui2_msg_count);

extern __s32 x_msg_q_attach (uintptr_t *   ph_msg_hdl,
                             const char*  ps_name);

extern __s32 x_msg_q_delete (uintptr_t  h_msg_hdl);

extern __s32 x_msg_q_send (uintptr_t     h_msg_hdl,
                           const void*  pv_msg,
                           size_t       z_size,
                           __u8        ui1_priority);

extern __s32 x_msg_q_receive (__u16*        pui2_index,
                              void*          pv_msg,
                              size_t*        pz_size,
                              uintptr_t *     ph_msg_q_mon_list,
                              __u16         ui2_msg_q_mon_count,
                              MSGQ_OPTION_T  e_options);

extern __s32 x_msg_q_receive_timeout (__u16 *          pui2_index,
                        void            *pv_msg,
                        size_t          *pz_size,
                        uintptr_t *       ph_msgq_hdl_list,
                        __u16          ui2_msgq_hdl_count,
                        __u32          ui4_time);

extern __s32 x_msg_q_num_msgs (uintptr_t h_msg_hdl,
                               __u16*   pui2_num_msgs);

extern __s32 x_msg_q_get_max_msg_size (uintptr_t  h_msg_hdl,
                                       size_t*   pz_maxsize);


__s32 StrToInt(const char* pszStr);



extern __s32 x_snprintf (char*        ps_str,
                         size_t       z_size,
                         const char*  ps_format,
                         ...);
#ifdef __linux__
#ifndef __ARM2__
#ifdef __KERNEL__
extern __s32 x_vsnprintf(char *ps_str, size_t z_size, const char *ps_format, VA_LIST vl);
#endif
#endif
#endif

#ifdef __KERNEL__
extern const char *basename(const char *full_name);
#endif

#ifndef FILE_ONLY
#define FILE_ONLY basename(__FILE__)
#endif

#endif

