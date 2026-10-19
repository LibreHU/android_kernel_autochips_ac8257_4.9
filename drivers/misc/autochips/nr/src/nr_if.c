/* Copyright Statement:
 *
 * This software/firmware and related documentation ("AutoChips Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to AutoChips Inc. and/or its licensors. Without
 * the prior written permission of AutoChips inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of AutoChips Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * AutoChips Inc. (C) 2018. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("AUTOCHIPS SOFTWARE")
 * RECEIVED FROM AUTOCHIPS AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. AUTOCHIPS EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES AUTOCHIPS PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE AUTOCHIPS SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN AUTOCHIPS
 * SOFTWARE. AUTOCHIPS SHALL ALSO NOT BE RESPONSIBLE FOR ANY AUTOCHIPS SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND AUTOCHIPS'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE AUTOCHIPS SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT AUTOCHIPS'S OPTION, TO REVISE OR REPLACE THE
 * AUTOCHIPS SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO AUTOCHIPS FOR SUCH AUTOCHIPS SOFTWARE AT ISSUE.
 */


#ifndef ATC_ARM2_LK_SUPPORT
/* kernel */
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/sched.h>/*wake_up_process*/
#include <linux/kthread.h>/*kthread_create*/
#include <linux/err.h>/*IS_ERR,PTR_ERR*/
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/semaphore.h>/*mutex*/
//#include <linux/stddef.h>
#include "x_os.h"
#include "x_typedef.h"
#include <mt-plat/aee.h>

#else
/* arm2 */
/*#include <kernel/wait.h>*/
#include <err.h>
#include <lib/fs.h>
#include <malloc.h>
/*#include <kernel/semaphore.h>*/ /*mutex*/
#include <platform/interrupts.h> /*for isr handle*/
#include <mt_irq.h>
#include <mt_typedefs.h>
#include <mt_gic.h>
#include <reserved_memory.h>
#include "platform/timer.h"

#endif

#include "nr_log.h"
#include "nr_drv.h"
#include "nr_hal.h"

#ifndef ATC_ARM2_LK_SUPPORT
int _u4NR_DBG_LVL = NR_LOG_LVL_DBG;
#else
int _u4NR_DBG_LVL = NR_LOG_LVL_INFO;
#endif

const char *_pcNrLogLevel[] = {
	"[NR] [ERR]",
	"[NR] [WARN]",
	"[NR] [INFO]",
	"[NR] [HAL]",
	"[NR] [DBG]",
	"[NR] [IRQ]",
};

#ifndef ATC_ARM2_LK_SUPPORT
static char nr_aee_string[100];
#define nr_aee_print(string, args...) do {\
	snprintf(nr_aee_string, 100, "[nr_if]"string, ##args); \
	aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_MMPROFILE_BUFFER, nr_aee_string, "[nr] error:"string, ##args);	\
} while (0)
#endif

static unsigned long nrResMemPa;
static unsigned long nrResMemVa;
static unsigned long nrResMemSize;
static NR_PRM_T gNrParam;
static NR_BUFFER_T gNrBuffer;
static NR_ALLC_BUF_T gNrAlloBuf;
static unsigned long nrGbgYaddr;
static unsigned long nrGbgCaddr;

bool fgWchLockInit = false;

#ifndef ATC_ARM2_LK_SUPPORT
static nrLock nrlock;
static struct mutex mtLock;
#define nrMtLock()      mutex_lock(&mtLock)
#define nrMtUnlock()    mutex_unlock(&mtLock)

void NrLockInit(void)
{
	if (!fgWchLockInit) {
		//spin_lock_init(&nrlock.lock);
		mutex_init(&mtLock);
		fgWchLockInit = true;
		//NR_LOG(NR_LOG_LVL_DBG, "NrLockInit %p \n", &nrlock.lock);
	}
}
#if 0
void SpinLockNr(void)
{
	spin_lock_irqsave(&nrlock.lock, nrlock.flags);
	nrlock.u4LockCnt++;
	NR_LOG(NR_LOG_LVL_IRQ, "[SpinLockNr] u4LockCnt=%d \n", (int)nrlock.u4LockCnt);
}

void SpinUnlockNr(void)
{
	nrlock.u4LockCnt--;
	spin_unlock_irqrestore(&nrlock.lock, nrlock.flags);
	NR_LOG(NR_LOG_LVL_IRQ, "[SpinUnlockNr] u4LockCnt=%d \n", (int)nrlock.u4LockCnt);
}
#endif
static HANDLE_T _hNrQueueBufferMsgQ;
static HANDLE_T _hNrISRMsgQ;
#endif

struct task_struct *hNrInst = NULL;

#ifndef ATC_ARM2_LK_SUPPORT
static wait_queue_head_t _hNrISRWorkQ;
static wait_queue_head_t _hNrRECWorkQ;
#endif

static atomic_t gWakeUpIsrWq;
static atomic_t gWakeUpRecWq;
static atomic_t gNrBusy;

#define NR_REC_BUF_CNT  5
static atomic_t nrRecBufCnt;
static int nrRecHeadIdx = 0, nrRecEndIdx = 0;

/*recive buffer queue, max buffer num:5, beause TVD/WCH_VDI max send buff num:5*/
static AVIN_BUFFER_T * nrRecBuffQueue[NR_REC_BUF_CNT];
static AVIN_BUFFER_T * nrUnuseRecBuff = NULL;

static AVIN_BUFFER_T nrSyncBuff[NR_BUF_MAX_CNT];
static unsigned char nrWriteGarbage = 0;

#define MSG_SIZE sizeof(unsigned int)
#define WCH_MSG_SIZE sizeof(WCHYCADDR)
#define MSG_CNT 10
#define MSG_QUEUE_BUFFER_NAME "nr queue buffer msgQ"
#define MSG_ISR_NAME "nr ISR msgQ"
#define NR_KERNEL_THREAD_NAME "NR kernel thread"
#define NR_Y_BUF_SIZE  720*576
#define NR_C_BUF_SIZE  720*288 // max support 420

static unsigned char u1enableskip = 1;
unsigned int _u4DramClk = 4; /*3(156M)  4(78M)*/

void NrSwitchBuffer(uintptr_t u4yaddr, uintptr_t u4caddr, unsigned int u4NrWrBufIdx);
void NrSwitchBuffer_Sync(AVIN_BUFFER_T * wchBuff, unsigned int u4NrWrBufIdx);
void nrRecBufQueAdd(AVIN_BUFFER_T * nrWchBuff);
AVIN_BUFFER_T* nrRecBufQueGetHead(void);
int nrRecBufQueCheckEmpty(void);

void NrIsrDeInit(void);
void NrIsrInit(void);
void nr_hw_Init(void);
int NrDumpReg(void);

#ifndef ATC_ARM2_LK_SUPPORT
static unsigned char nrIsrTimeout = 0;

int NrUpdateAddressThread(void* data)
{
	unsigned int u4NrWrBufIdx = 0;
	NR_PRM_T * pNrParam = &gNrParam;
	PNR_ALLC_BUF_T pNrBuffer = &gNrAlloBuf;
	long ret = -1;
	AVIN_BUFFER_T * nrTrigBuff = NULL;

	NR_LOG(NR_LOG_LVL_INFO, "NrUpdateAddressThread! \n");

	for(;;){
		if (kthread_should_stop()){
			break;
		}

		//wait wch buffer rec done
		#ifndef ATC_ARM2_LK_SUPPORT
		wait_event_interruptible(_hNrRECWorkQ, atomic_read(&gWakeUpRecWq));
		atomic_set(&gWakeUpRecWq, 0);

		/*wait nr isr done,  5ms timeout*/
		ret = wait_event_interruptible_timeout(_hNrISRWorkQ,
				atomic_read(&gWakeUpIsrWq), msecs_to_jiffies(5));
		atomic_set(&gWakeUpIsrWq, 0);
		#endif

		nrMtLock();
		nrTrigBuff = nrRecBufQueGetHead();
		if( NULL == nrTrigBuff ){
			NR_LOG(NR_LOG_LVL_ERR, "nr Dequeue receive buffer error! \n");
			nrMtUnlock();
			if (0 == atomic_read(&gNrBusy)) {//when hw is not busy, wake recWorkQ
				atomic_set(&gWakeUpIsrWq, 1);
				wake_up_interruptible(&_hNrISRWorkQ);
			}
			continue;
		}

		if (0 == ret) { // timeout
			NR_LOG(NR_LOG_LVL_ERR, "Nr wait ISR timeout\n");
			int i = 0;
			for(i = nrRecHeadIdx; i <= nrRecEndIdx; i++) {
				if(NULL != nrRecBuffQueue[i]) {
					NR_LOG(NR_LOG_LVL_ERR, "nr rec buffer index %d, status %d, y_addr %p!\r\n",
						i,atomic_read(&nrRecBuffQueue[i]->status),(void*)nrRecBuffQueue[i]->y_addr);
				}
			}

			NrDumpReg();
			_NrCloseClk();
			NrIsrDeInit();
			nrMtUnlock();
			nr_hw_Init();
			nrMtLock();
			_NrSetParam(pNrParam);
			_NrClearInterrupteStatus();
			//set rec unuseful buffer free
			if (NULL != nrUnuseRecBuff) {
				atomic_set(&nrUnuseRecBuff->status, AVIN_BUF_FREE);
				NR_LOG(NR_LOG_LVL_ERR, "after timeout, set free idex is %d, y_addr is %p \n",
					nrUnuseRecBuff->idex,(void*)nrUnuseRecBuff->y_addr);
			}
			nrIsrTimeout = 1;
			//nr_aee_print("Nr ISR timeout!!!");
		} else {
			nrIsrTimeout = 0;
		}

		//update nr buffer
		if (NULL != nrTrigBuff) {
			if(1 == nrIsrTimeout){ // trigger but do not swich buffer
				pNrBuffer->u4BufIdx = (pNrBuffer->u4BufIdx - 1 + NR_BUF_MAX_CNT) % NR_BUF_MAX_CNT;
				NR_LOG(NR_LOG_LVL_INFO, "NR still write index %d because of timeout\n", pNrBuffer->u4BufIdx);
			}
			NrSwitchBuffer_Sync(nrTrigBuff, pNrBuffer->u4BufIdx);
		}

		if (1 == nrWriteGarbage || 1 == nrIsrTimeout) {
			NR_LOG(NR_LOG_LVL_ERR, "nrWriteGarbage(%d) nrIsrTimeout(%d), skip send buffer\n", nrWriteGarbage, nrIsrTimeout);
			nrMtUnlock();
			continue;
		}

		if (u1enableskip) {
			NR_LOG(NR_LOG_LVL_INFO, "NR skip %d frame \n", u1enableskip);
			u1enableskip--;
		} else {
			u4NrWrBufIdx = (pNrBuffer->u4BufIdx -2 + NR_BUF_MAX_CNT) % NR_BUF_MAX_CNT; // ready index
			atomic_set(&nrSyncBuff[u4NrWrBufIdx].status, AVIN_BUF_BUSY); // set buffer busy
            #ifndef ATC_ARM2_LK_SUPPORT
			//spin_lock(&nrlock.lock); //Old option:  SpinLockNr();
			#endif
			if (pNrParam->GetNrBufInfo){
				pNrParam->GetNrBufInfo(&nrSyncBuff[u4NrWrBufIdx]);
			}
			#ifndef ATC_ARM2_LK_SUPPORT
			//spin_unlock(&nrlock.lock); //Old option:  SpinUnlockNr();
			#endif
		}
		nrMtUnlock();
	}
	complete_and_exit(NULL, 0);

	return 0;
}

static unsigned int NRdate = 168;
void NrKernelThreadInit(void)
{
	NR_LOG(NR_LOG_LVL_INFO, "NrKernelThreadInit begin! \n");

	//queue buffer done
	/*
	if (OSR_OK != x_msg_q_create(&_hNrQueueBufferMsgQ, MSG_QUEUE_BUFFER_NAME, WCH_MSG_SIZE, MSG_CNT)) {
		NR_LOG(NR_LOG_LVL_ERR, "_hNrQueueBufferMsgQ	x_msg_q_create fail!\r\n");
		return;
	}
	NR_LOG(NR_LOG_LVL_INFO, "  NrKernelThreadInit Enter _hNrQueueBufferMsgQ: %p, *(&_hNrQueueBufferMsgQ): %p!\r\n",
		_hNrQueueBufferMsgQ, *(&_hNrQueueBufferMsgQ));
	*/

	//ISR done
	/*
	if (OSR_OK != x_msg_q_create(&_hNrISRMsgQ, MSG_ISR_NAME, MSG_SIZE, MSG_CNT)) {
		NR_LOG(NR_LOG_LVL_ERR, "_hNrISRMsgQ	x_msg_q_create fail!\r\n");
		return;
	}

	NR_LOG(NR_LOG_LVL_INFO, "  NrKernelThreadInit Enter _hNrISRMsgQ: %p, *(&_hNrISRMsgQ): %p!\r\n",
		_hNrISRMsgQ, *(&_hNrISRMsgQ));
	*/
    #ifndef ATC_ARM2_LK_SUPPORT
	init_waitqueue_head(&_hNrISRWorkQ); //isr wait event
	init_waitqueue_head(&_hNrRECWorkQ); //receive buffer wait event
    #endif

	//kernel thread
	hNrInst = kthread_create(NrUpdateAddressThread, (void*)&NRdate, NR_KERNEL_THREAD_NAME);
	if (!IS_ERR(hNrInst)) {
		NR_LOG(NR_LOG_LVL_INFO, "NrKernelThreadInit create thread success = 0x%p \n", hNrInst);
		wake_up_process(hNrInst);
	} else{
		NR_LOG(NR_LOG_LVL_ERR, " NrKernelThreadInit kthread_create fail!\r\n");
		hNrInst = NULL;
	}
	NR_LOG(NR_LOG_LVL_INFO, "NrKernelThreadInit end! \n");

}
#endif

void setNrReservemem(unsigned long pa, unsigned long va, unsigned int size)
{
	nrResMemPa = pa;
	nrResMemVa = va;
	nrResMemSize = size;

	NR_LOG(NR_LOG_LVL_INFO, "setNrReservemem Pa:%lx, Size:%lx, Va:%lx\n",
				nrResMemPa, nrResMemSize, nrResMemVa);
}

int NrAllocateBuff(unsigned int u4FrameWidth, unsigned int u4FrameHeight)
{
	int nrcnt = 0;
	PNR_ALLC_BUF_T pNrAlloBuf = &gNrAlloBuf;

	if (u4FrameWidth == 0 || u4FrameHeight == 0) {
		NR_LOG(NR_LOG_LVL_INFO, "NrAllocateBuff error,width and height is 0 \n");
		return -1;
	}

	pNrAlloBuf->u4BufIdx = 0;
	for (; nrcnt < NR_BUF_MAX_CNT; nrcnt++) {
		//pNrAlloBuf->u4YBuf[nrcnt] = nrResMemPa + nrcnt * u4FrameWidth * u4FrameHeight * 3/2;
		//pNrAlloBuf->u4CBuf[nrcnt] = pNrAlloBuf->u4YBuf[nrcnt] + u4FrameWidth * u4FrameHeight;
		/* for fix 720*480 Y data error , maybe hardware issue, change alloc memory to static 720*576 */
		pNrAlloBuf->u4YBuf[nrcnt] = nrResMemPa + nrcnt * ( NR_Y_BUF_SIZE + NR_C_BUF_SIZE );
		pNrAlloBuf->u4CBuf[nrcnt] = pNrAlloBuf->u4YBuf[nrcnt] + NR_Y_BUF_SIZE;
	}

	/* init garbage buffer addr and sync buffer info*/
	/*nrGbgYaddr = nrResMemPa + NR_BUF_MAX_CNT * u4FrameWidth * u4FrameHeight * 3/2;*/
	/*nrGbgCaddr = nrGbgYaddr + u4FrameWidth * u4FrameHeight;*/

	nrGbgYaddr = pNrAlloBuf->u4CBuf[NR_BUF_MAX_CNT -1] + NR_C_BUF_SIZE;
	nrGbgCaddr = nrGbgYaddr + NR_Y_BUF_SIZE;

	for (nrcnt = 0; nrcnt < NR_BUF_MAX_CNT; nrcnt++) {
		nrSyncBuff[nrcnt].y_addr = pNrAlloBuf->u4YBuf[nrcnt];
		nrSyncBuff[nrcnt].c_addr = pNrAlloBuf->u4CBuf[nrcnt];
		nrSyncBuff[nrcnt].idex = nrcnt;
		atomic_set(&nrSyncBuff[nrcnt].status, AVIN_BUF_FREE);
	}

	NR_LOG(NR_LOG_LVL_INFO, "NR buffer start Y address: 0x%lx\n",
					pNrAlloBuf->u4YBuf[0]);
	NR_LOG(NR_LOG_LVL_INFO, "NR buffer last  C address: 0x%lx\n",
					pNrAlloBuf->u4CBuf[NR_BUF_MAX_CNT - 1]);
	return 0;
}

#ifndef ATC_ARM2_LK_SUPPORT
void nrClearBufQueue(void)
{
	AVIN_BUFFER_T* pBuf = NULL;
	while(0 != nrRecBufQueCheckEmpty()) {
		pBuf = nrRecBufQueGetHead();
		atomic_set(&pBuf->status, AVIN_BUF_FREE);
	}
}
irqreturn_t NrIsr(int u2Vector, void *dev_id)
{
	// clear isr first
	_NrClearInterrupteStatus();

	atomic_set(&gNrBusy, 0);
	//set rec unuseful buffer free
	if (NULL != nrUnuseRecBuff) {
		atomic_set(&nrUnuseRecBuff->status, AVIN_BUF_FREE);
	}

	//wake up isr event
	atomic_set(&gWakeUpIsrWq, 1);
	wake_up_interruptible(&_hNrISRWorkQ);

	//wake up rec buff event if buffer queue remain
	if( 0 != nrRecBufQueCheckEmpty() ){
		atomic_set(&gWakeUpRecWq, 1);
		wake_up_interruptible(&_hNrRECWorkQ);
	}

	return IRQ_HANDLED;
}
#else

void nr_irq_handler(void* arg)
{
	unsigned int u4NrWrBufIdx = 0;
	NR_PRM_T * pNrParam = &gNrParam;
	PNR_ALLC_BUF_T pNrBuffer = &gNrAlloBuf;

	atomic_set(&gNrBusy, 0);
	// clear isr first
	_NrClearInterrupteStatus();

	//set rec unuseful buffer free
	if (NULL != nrUnuseRecBuff) {
		atomic_set(&nrUnuseRecBuff->status, AVIN_BUF_FREE);
		NR_LOG(NR_LOG_LVL_DBG, " nr_irq_handler , set free idex is %d, y_addr is %p \n",
			nrUnuseRecBuff->idex,(void*)nrUnuseRecBuff->y_addr);
	}

	if (1 == nrWriteGarbage ) {
		NR_LOG(NR_LOG_LVL_ERR, "nrWriteGarbage(%d) skip send buffer\n", nrWriteGarbage);
		//return;
	} else {
		if (u1enableskip) {
			NR_LOG(NR_LOG_LVL_INFO, "NR skip %d frame \n", u1enableskip);
			u1enableskip--;
		} else {
			u4NrWrBufIdx = (pNrBuffer->u4BufIdx -1 + NR_BUF_MAX_CNT) % NR_BUF_MAX_CNT; // ready index
			atomic_set(&nrSyncBuff[u4NrWrBufIdx].status, AVIN_BUF_BUSY); // set buffer busy
			if (pNrParam->GetNrBufInfo){
				pNrParam->GetNrBufInfo(&nrSyncBuff[u4NrWrBufIdx]);
			}
		}
	}

        //if more buffer in recQ, switch buffer next
	AVIN_BUFFER_T * nrTrigBuff = nrRecBufQueGetHead();
	if (NULL != nrTrigBuff) {
		NR_LOG(NR_LOG_LVL_DBG, " nr_irq_handler more buffer idex is %d, y_addr is %p \n",
			nrTrigBuff->idex,(void*)nrTrigBuff->y_addr);
		NrSwitchBuffer_Sync(nrTrigBuff, pNrBuffer->u4BufIdx);
	} else {
		NR_LOG(NR_LOG_LVL_DBG, "there is no more nr buffer in recQueue \n");
	}

}
#endif

unsigned int fgReqirq = 0;
unsigned int nrParamInit = 0;
#ifndef ATC_ARM2_LK_SUPPORT
extern unsigned int IRQVECTOR;
#else
unsigned int IRQVECTOR = ARM9_NR_IRQ_ID; //defined in mt_irq.h
#endif

void NrIsrInit(void)
{
	if (fgReqirq == 0) {
		#ifndef ATC_ARM2_LK_SUPPORT
		int ret = -1;
		ret = request_irq(IRQVECTOR, (irq_handler_t)NrIsr, IRQF_TRIGGER_LOW, "NR ISR", NULL);
		if (ret) {
			NR_LOG(NR_LOG_LVL_ERR, "NR request_irq error!\n");
		}
		else {
			fgReqirq = 1;
		}
		#else
		mt_irq_unmask(IRQVECTOR);
		fgReqirq = 1;
		#endif
	}
}

void NrIsrDeInit(void)
{
	if (fgReqirq == 1) {
		#ifndef ATC_ARM2_LK_SUPPORT
		free_irq(IRQVECTOR, NULL);
		#else
		mt_irq_mask(IRQVECTOR);
		#endif
		NR_LOG(NR_LOG_LVL_INFO, "NR free_irq\n");
		fgReqirq = 0;
	}
}

void NrParamInit(void)
{
	int i = 0;
	if (nrParamInit == 0) {
		nrUnuseRecBuff = NULL;
		nrRecHeadIdx = 0;
		nrRecEndIdx = 0;
		atomic_set(&nrRecBufCnt, 0);

		for (i = 0; i < NR_REC_BUF_CNT ; i++) {
			nrRecBuffQueue[i] = NULL;
		}

		nrParamInit = 1;
		NR_LOG(NR_LOG_LVL_INFO, "NrSwInit done\n");
	}
}

int nrRecBufQueCheckEmpty(void)
{
	if (0 == atomic_read(&nrRecBufCnt)) { //empty
		return 0;
	}
	else { //not empty
		NR_LOG(NR_LOG_LVL_ERR, "nr recv buf queue has (%d)buffer remain!\r\n", atomic_read(&nrRecBufCnt));
		return atomic_read(&nrRecBufCnt);
	}
}

void nrRecBufQueAdd(AVIN_BUFFER_T * nrWchBuff)
{
	if (1 != atomic_read(&nrWchBuff->status)) {
		NR_LOG(NR_LOG_LVL_ERR, "nr inqueue rec_buffer status(%d) error!\r\n", atomic_read(&nrWchBuff->status));
		#ifndef ATC_ARM2_LK_SUPPORT
		vss_aee_print("nr inqueue rec_buffer status(%d) error! \n", atomic_read(&nrWchBuff->status));
		#endif
	}
	if (atomic_read(&nrRecBufCnt) < NR_REC_BUF_CNT) {
		nrRecBuffQueue[nrRecEndIdx] = nrWchBuff;
		nrRecEndIdx = (nrRecEndIdx + 1 + NR_REC_BUF_CNT) % NR_REC_BUF_CNT;
		atomic_inc(&nrRecBufCnt);
	}
	else {
		NR_LOG(NR_LOG_LVL_ERR, "nr rec buffer queue is full(%d), add error!\r\n", atomic_read(&nrRecBufCnt));
		#ifndef ATC_ARM2_LK_SUPPORT
		vss_aee_print("nr rec buffer queue is full(%d), add error!\r\n", atomic_read(&nrRecBufCnt));
		#endif
	}
}

AVIN_BUFFER_T* nrRecBufQueGetHead(void)
{
	int headtmp = 0;
	if (0 == atomic_read(&nrRecBufCnt)) {
		return NULL;
	}
	headtmp = nrRecHeadIdx;
	nrRecHeadIdx = (nrRecHeadIdx + 1 + NR_REC_BUF_CNT) % NR_REC_BUF_CNT;
	atomic_dec(&nrRecBufCnt);
	return nrRecBuffQueue[headtmp];
}

static unsigned char kernelnrtriggered = 0;
static unsigned char triggerErrorHandle = 0;

#ifdef ATC_ARM2_LK_SUPPORT
// for arm2 kernel thread init / resmem init
static unsigned char nrarm2init = 0;
static unsigned long nrarm2resmempa = 0;
static unsigned long nrarm2resmemva = 0; // no need
static unsigned long nrarm2resmemsize = 0;

void initArm2ResMem(void)
{
	nrarm2resmempa = get_reserved_memory_node_addr("nr-reserved-memory");
	nrarm2resmemsize = get_reserved_memory_node_size("nr-reserved-memory");
}
#endif

void nr_hw_Init(void)
{
	NR_LOG(NR_LOG_LVL_INFO, "NR Init Start, Version is [2019.08.15]\n");
#ifndef ATC_ARM2_LK_SUPPORT
	int timeout = 0;
	NrLockInit();
#else
	if(0 == nrarm2init){
		initArm2ResMem();
		setNrReservemem(nrarm2resmempa, nrarm2resmemva, nrarm2resmemsize);
		//NrKernelThreadInit();
		nrarm2init = 1;
	}
#endif

#ifndef ATC_ARM2_LK_SUPPORT
	nrMtLock();
#endif

#ifndef ATC_ARM2_LK_SUPPORT
	nrClearBufQueue();
	while(atomic_read(&gNrBusy)) {
		NR_LOG(NR_LOG_LVL_INFO, "NR HW gNrBusy,Init sleep\n");
		if(timeout < 20) {
			msleep(1);
		} else {
			NR_LOG(NR_LOG_LVL_ERR, "NR Init wait gNrBusy timeout!\n");
			atomic_set(&gNrBusy, 0);
			if (NULL != nrUnuseRecBuff) {
				atomic_set(&nrUnuseRecBuff->status, AVIN_BUF_FREE);
				NR_LOG(NR_LOG_LVL_ERR, "after init timeout set free idex %d, y_addr %p \n",
					nrUnuseRecBuff->idex,(void*)nrUnuseRecBuff->y_addr);
			}

			break;
		}
		timeout++;
	}
#endif
	//Set NR Clk
	_NrSetClk(_u4DramClk);

	/* vdp dram bridge reset*/
	_NrVdpDramReset();

	/* Clear NR Interrupt*/
	_NrClearInterrupteStatus();

	//Reset NR module
	_NrHwReset();

	NrIsrInit();
	//sw param init
	NrParamInit();

	//ParaInit & Set NR Para
	_NrParaInit();
	_NrParaSet(1,3,3,3);
#ifndef ATC_ARM2_LK_SUPPORT
	nrMtUnlock();
#endif
	NR_LOG(NR_LOG_LVL_INFO, "NR Init Done. \n");
}

void nr_hw_deInit(void)
{
	int timeout = 0;

#ifndef ATC_ARM2_LK_SUPPORT
	nrMtLock();
	NR_LOG(NR_LOG_LVL_INFO, "NR deInit in\n");
	nrClearBufQueue();
#endif

	while(atomic_read(&gNrBusy)) {
		NR_LOG(NR_LOG_LVL_INFO, "NR HW gNrBusy, deInit sleep\n");
		if(timeout < 20) {
#ifndef ATC_ARM2_LK_SUPPORT
			msleep(1);
#else

			mdelay(1);

#endif
		} else {
			NR_LOG(NR_LOG_LVL_ERR, "NR deInit wait gNrBusy timeout!\n");
			atomic_set(&gNrBusy, 0);
			if (NULL != nrUnuseRecBuff) {
				atomic_set(&nrUnuseRecBuff->status, AVIN_BUF_FREE);
			}
			break;
		}
		timeout++;
	}
#ifndef ATC_ARM2_LK_SUPPORT
	//spin_lock(&nrlock.lock); //Old option:  SpinLockNr();
	memset(&gNrParam, 0, sizeof(NR_PRM_T));
	//spin_unlock(&nrlock.lock); //Old option:  SpinUnlockNr();
	/* Do not close arm1 nr clock, for fix 3 green vertical bar when ipo repeatedly*/
	/*_NrCloseClk();*/
#else
	_NrCloseClk();
#endif

	NrIsrDeInit();
	triggerErrorHandle = 0;
	u1enableskip = 1;
	nrParamInit = 0;
#ifndef ATC_ARM2_LK_SUPPORT
	nrMtUnlock();
#endif
}

void nr_hw_config(NR_PRM_T * ptNrPrm)
{
	NR_PRM_T *pNrParam = NULL;
	unsigned int u4WAlign = 16, u4HAlign = 32;
	unsigned int u4AlignWidth, u4AlignHeight;

	if (ptNrPrm == NULL) {
		NR_LOG(NR_LOG_LVL_ERR, "nr_hw_config's parameter:ptNrPrm is NULL. \n");
		return;
	}
#ifndef ATC_ARM2_LK_SUPPORT
	nrMtLock();
#endif
	pNrParam = &gNrParam;
	memcpy(&gNrParam, ptNrPrm, sizeof(NR_PRM_T));

	//align picture size
	u4AlignWidth = ((pNrParam->u4PicWidth + (u4WAlign - 1)) / u4WAlign ) * u4WAlign;
	u4AlignHeight =((pNrParam->u4PicHeight + (u4HAlign - 1)) / u4HAlign ) * u4HAlign;
	pNrParam->u4FrameWidth = u4AlignWidth;
	pNrParam->u4FrameHeight = u4AlignHeight;

	//allocate nr buffer
	NrAllocateBuff(pNrParam->u4FrameWidth, pNrParam->u4FrameHeight);

	_NrSetParam(pNrParam);
#ifndef ATC_ARM2_LK_SUPPORT
	nrMtUnlock();
#endif
}

void nr_hw_trigger(void)
{
	NR_BUFFER_T * ptNrBuf = &gNrBuffer;
	PNR_ALLC_BUF_T pNrAlloBuf = &gNrAlloBuf;
	if (!triggerErrorHandle) {
		kernelnrtriggered = 1;
		triggerErrorHandle = 1;
		ptNrBuf->u4CurrRdYAddr = pNrAlloBuf->u4YBuf[4];
		ptNrBuf->u4CurrRdCAddr = pNrAlloBuf->u4CBuf[4];
		ptNrBuf->u4LastRdYAddr = pNrAlloBuf->u4YBuf[4];
		ptNrBuf->u4LastRdCAddr = pNrAlloBuf->u4CBuf[4];
		ptNrBuf->u4CurrWrYAddr = pNrAlloBuf->u4YBuf[0];
		ptNrBuf->u4CurrWrCAddr = pNrAlloBuf->u4CBuf[0];

		atomic_set(&gNrBusy, 1);
		_NrSetBufTrigger(ptNrBuf);
	}

	NR_LOG(NR_LOG_LVL_INFO, "NrProcess trigger begin \n");
}

void nr_buffer_receive_sync(void * nr_rec_buffer)
{
	nrRecBufQueAdd(nr_rec_buffer); // put buffer in queue

	#ifndef ATC_ARM2_LK_SUPPORT
	atomic_set(&gWakeUpRecWq, 1);
	wake_up_interruptible(&_hNrRECWorkQ);
	#else
	AVIN_BUFFER_T * nrTrigBuff = NULL;
	PNR_ALLC_BUF_T pNrBuffer = &gNrAlloBuf;
	if (0 == atomic_read(&gNrBusy)) {//when hw is not busy, switch buffer to hw
		nrTrigBuff = nrRecBufQueGetHead();

		//update nr buffer
		if (NULL != nrTrigBuff) {
			NrSwitchBuffer_Sync(nrTrigBuff, pNrBuffer->u4BufIdx);
		} else {
			NR_LOG(NR_LOG_LVL_ERR, "nr Dequeue receive buffer error! \n");
		}
	}
	#endif

}


static uintptr_t u4LastRdYAddr = 0;
static uintptr_t u4LastRdCAddr = 0;
static AVIN_BUFFER_T * nrLastRecBuff = NULL;


void NrSwitchBuffer(uintptr_t u4yaddr, uintptr_t u4caddr, unsigned int u4NrWrBufIdx)
{
	NR_BUFFER_T * ptNrBuf = &gNrBuffer;
	PNR_ALLC_BUF_T pNrAlloBuf = &gNrAlloBuf;

	if (kernelnrtriggered) {
		u4LastRdYAddr = u4yaddr;
		u4LastRdCAddr = u4caddr;
		kernelnrtriggered = 0;
	}

	ptNrBuf->u4CurrRdYAddr = u4yaddr;
	ptNrBuf->u4CurrRdCAddr = u4caddr;
	ptNrBuf->u4LastRdYAddr = u4LastRdYAddr;
	ptNrBuf->u4LastRdCAddr = u4LastRdCAddr;
	ptNrBuf->u4CurrWrYAddr = pNrAlloBuf->u4YBuf[u4NrWrBufIdx];
	ptNrBuf->u4CurrWrCAddr = pNrAlloBuf->u4CBuf[u4NrWrBufIdx];
	u4LastRdYAddr = u4yaddr;
	u4LastRdCAddr = u4caddr;

	//update nr buffer index
	pNrAlloBuf->u4BufIdx++;
	if (pNrAlloBuf->u4BufIdx == NR_BUF_MAX_CNT){
		pNrAlloBuf->u4BufIdx = 0;
	}

	_NrSetBufTrigger(ptNrBuf);

}

void NrSwitchBuffer_Sync(AVIN_BUFFER_T * wchBuff, unsigned int u4NrWrBufIdx)
{
	NR_BUFFER_T * ptNrBuf = &gNrBuffer;
	PNR_ALLC_BUF_T pNrAlloBuf = &gNrAlloBuf;

	if (0 == kernelnrtriggered) {
		nrUnuseRecBuff = nrLastRecBuff;
	}

	if (kernelnrtriggered) {
		u4LastRdYAddr = wchBuff->y_addr;
		u4LastRdCAddr = wchBuff->c_addr;
		kernelnrtriggered = 0;
	}

	ptNrBuf->u4CurrRdYAddr = wchBuff->y_addr;
	ptNrBuf->u4CurrRdCAddr = wchBuff->c_addr;
	ptNrBuf->u4LastRdYAddr = u4LastRdYAddr;
	ptNrBuf->u4LastRdCAddr = u4LastRdCAddr;

	if (AVIN_BUF_BUSY == atomic_read(&nrSyncBuff[u4NrWrBufIdx].status)) { // no free buffer , write to garbage buffer
		NR_LOG(NR_LOG_LVL_ERR, " nr buffer index(%d) is busy! \n", u4NrWrBufIdx);
		#ifndef ATC_ARM2_LK_SUPPORT
		vss_aee_print(" nr buffer index(%d) is busy! \n", u4NrWrBufIdx);
		#endif
		ptNrBuf->u4CurrWrYAddr = nrGbgYaddr;
		ptNrBuf->u4CurrWrCAddr = nrGbgCaddr;
		nrWriteGarbage = 1;

	} else {
		ptNrBuf->u4CurrWrYAddr = pNrAlloBuf->u4YBuf[u4NrWrBufIdx];
		ptNrBuf->u4CurrWrCAddr = pNrAlloBuf->u4CBuf[u4NrWrBufIdx];
		nrWriteGarbage = 0;
		pNrAlloBuf->u4BufIdx++;
		if (pNrAlloBuf->u4BufIdx == NR_BUF_MAX_CNT){
			pNrAlloBuf->u4BufIdx = 0;
		}
	}

	u4LastRdYAddr = wchBuff->y_addr;
	u4LastRdCAddr = wchBuff->c_addr;
	nrLastRecBuff = wchBuff; // remeber wch buffer

	atomic_set(&gNrBusy, 1);
	_NrSetBufTrigger(ptNrBuf);

}


int NrBypass(int fgBypass)
{
	if (fgBypass < 0) {
		NR_LOG(NR_LOG_LVL_ERR,"NrBypass fgBypass err:%d \n", fgBypass);
		return -1;
	}
	_NrBypass((unsigned int)fgBypass);

	return 0;
}

int NrEnable3dFunc(int fgBypass)
{
	if (fgBypass < 0) {
		NR_LOG(NR_LOG_LVL_ERR,"NrEnable3dFunc fgBypass err:%d \n", fgBypass);
		return -1;
	}
	_NrEnable3dFunc((unsigned int)fgBypass);

	return 0;
}

int NrSetSwapMode(int swapMode)
{
	if (swapMode < 0 || swapMode > 6){
		NR_LOG(NR_LOG_LVL_ERR,"NrSetSwapMode swapMode err:%d \n", swapMode);
		return -1;
	}
	_NrSetSwapMode((unsigned int)swapMode);

	return 0;
}

int NrSetLevel(unsigned int u4Strength,unsigned int u4FNRStrength,
					unsigned int u4MNRStrength,unsigned int u4BNRStrength)
{
	if (u4Strength > 3 || u4FNRStrength > 3 ||
		u4MNRStrength > 3 || u4BNRStrength > 3) {

		NR_LOG(NR_LOG_LVL_ERR,"NrSetSwapMode swapMode err! \n");
		return -1;
	}
	_NrParaSet(u4Strength, u4FNRStrength, u4MNRStrength, u4BNRStrength);
	_NrLevelSet(u4Strength, u4FNRStrength, u4MNRStrength, u4BNRStrength);

	return 0;
}

#ifndef ATC_ARM2_LK_SUPPORT
void nr_dump_template(const unsigned long virt_addr, const unsigned int size, const unsigned char nrDumpFile[])
{
	struct file *fp = NULL;
	mm_segment_t old_fs = 0;
	unsigned int ret = 0;
	loff_t pos = 0;
	fp = filp_open(nrDumpFile, O_CREAT | O_RDWR, 0777);
	if (IS_ERR(fp)) {
		ret = PTR_ERR(fp);
		NR_LOG(NR_LOG_LVL_ERR, "[nr_dump_template] open %s error! Error Num[%d]\r\n", nrDumpFile, ret);
		return;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	pos = 0;
	ret = vfs_write(fp, (char __user *)virt_addr, size, &pos);

	filp_close(fp, NULL);
	set_fs(old_fs);
}


int NrDumpBuffer(unsigned int cnt)
{
	NR_LOG(NR_LOG_LVL_INFO, "NrDumpBuffer start!\n");

	unsigned int i= 0;
	unsigned long dump_addr = 0;
	unsigned int nrReadyIdx = 0;
	unsigned char nrDumpFile[256] = "";

	unsigned int y_size = 720*576;
	unsigned int c_size = 720*576/2;
	PNR_ALLC_BUF_T pNrAlloBuf = &gNrAlloBuf;

	unsigned int paddr_in_y = 0;
	unsigned int paddr_in_c = 0;
	unsigned int paddr_out_y = 0;
	unsigned int paddr_out_c = 0;

	for( ; i < cnt; i++){
		 /**** dump input buffer ****/

		if (NULL != nrRecBuffQueue[nrRecEndIdx]) {
			paddr_in_y = nrRecBuffQueue[nrRecEndIdx]->y_addr;
			paddr_in_c = nrRecBuffQueue[nrRecEndIdx]->c_addr;

			dump_addr = (unsigned long)ioremap(paddr_in_y, y_size);
			sprintf(nrDumpFile, "/sdcard/nr/nr-%d_in_y.bin", i);
			nr_dump_template(dump_addr, y_size, nrDumpFile);

			dump_addr = (unsigned long)ioremap(paddr_in_c, c_size);
			sprintf(nrDumpFile, "/sdcard/nr/nr-%d_in_c.bin", i);
			nr_dump_template(dump_addr, c_size, nrDumpFile);
		}

		/**** dump output buffer ****/
		nrReadyIdx = (pNrAlloBuf->u4BufIdx -1 + NR_BUF_MAX_CNT) % NR_BUF_MAX_CNT;
		 NR_LOG(NR_LOG_LVL_INFO, "NrDumpBuffer nrReadyIdx(0x%x)!\n", nrReadyIdx);
		paddr_out_y = pNrAlloBuf->u4YBuf[nrReadyIdx];
		paddr_out_c = pNrAlloBuf->u4CBuf[nrReadyIdx];

		NR_LOG(NR_LOG_LVL_INFO, "NrDumpBuffer paddr_out_y(0x%x)!\n", paddr_out_y);
		NR_LOG(NR_LOG_LVL_INFO, "NrDumpBuffer paddr_out_c(0x%x)!\n", paddr_out_c);

		sprintf(nrDumpFile, "/sdcard/nr/nr-%d_out_y.bin", i);
		dump_addr = nrResMemVa + paddr_out_y -nrResMemPa;
		nr_dump_template(dump_addr, y_size, nrDumpFile);

		sprintf(nrDumpFile, "/sdcard/nr/nr-%d_out_c.bin", i);
		dump_addr = nrResMemVa + paddr_out_c -nrResMemPa;
		nr_dump_template(dump_addr, c_size, nrDumpFile);
	}

	NR_LOG(NR_LOG_LVL_INFO, "NrDumpBuffer end!\n");

	return 0;
}

extern void __iomem* nr_reg;
extern void __iomem* clock_reg;
extern void __iomem* vdpsys_reg;

int NrDumpReg(void){
	NR_LOG(NR_LOG_LVL_INFO, "NrDumpReg start!\n");

	unsigned long dump_addr = 0;
	unsigned int dump_size = 0;
	unsigned char nrDumpFile[256] = "";

	 /**** dump nr inside register ****/
	dump_addr = (unsigned long)nr_reg;
	dump_size = 0xd00;
	sprintf(nrDumpFile, "/sdcard/nr_register.bin");
	nr_dump_template(dump_addr, dump_size, nrDumpFile);

	/**** dump nr vdpsys register ****/
	dump_addr = (unsigned long)vdpsys_reg;
	dump_size = 0x110;
	sprintf(nrDumpFile, "/sdcard/vdpsys_register.bin");
	nr_dump_template(dump_addr, dump_size, nrDumpFile);

	/**** printf nr self & clock_reg & vdpsys register ****/
    _NrPrintReg();

	NR_LOG(NR_LOG_LVL_INFO, "NrDumpReg end!\n");
	return 0;
}
#endif


