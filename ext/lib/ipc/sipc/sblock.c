/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/sys_log.h>
#include <uwp_hal.h>
#include <string.h>

#include "sipc.h"
#include "sipc_priv.h"
#include "sblock.h"

static struct sblock_mgr sblocks[SIPC_ID_NR][SMSG_CH_NR];
uint8_t cmd_hdr1[] = 
{0x10,0x1,0xc,0x0,0x55,0x49,0x0,0x0,0x0,0x0,0x0,0x0};

int sblock_get(uint8_t dst, uint8_t channel, struct sblock *blk, int timeout);
int sblock_send(uint8_t dst, uint8_t channel,uint8_t prio, struct sblock *blk);

static int sblock_recover(uint8_t dst, uint8_t channel)
{
	struct sblock_mgr *sblock = &sblocks[dst][channel];
	struct sblock_ring *ring = NULL;
	volatile struct sblock_ring_header *ringhd = NULL;
	volatile struct sblock_ring_header *poolhd = NULL;
	//unsigned long pflags, qflags;
	int i, j;

	if (!sblock) {
		return -ENODEV;
	}
	ring = &sblock->ring;
	ringhd = (volatile struct sblock_ring_header *)(&ring->header->ring);
	poolhd = (volatile struct sblock_ring_header *)(&ring->header->pool);

	sblock->state = SBLOCK_STATE_IDLE;

    //spin_lock_irqsave(&ring->r_txlock, pflags);
	/* clean txblks ring */
	ringhd->txblk_wrptr = ringhd->txblk_rdptr;

	//spin_lock_irqsave(&ring->p_txlock, qflags);

    	/* recover txblks pool */
	poolhd->txblk_rdptr = poolhd->txblk_wrptr;
	for (i = 0, j = 0; i < poolhd->txblk_count; i++) {
		if (ring->txrecord[i] == SBLOCK_BLK_STATE_DONE) {
			ring->p_txblks[j].addr = i * sblock->txblksz + poolhd->txblk_addr;
			ring->p_txblks[j].length = sblock->txblksz;
			poolhd->txblk_wrptr = poolhd->txblk_wrptr + 1;
			j++;
		}
	}
    //spin_unlock_irqrestore(&ring->p_txlock, qflags);
	//spin_unlock_irqrestore(&ring->r_txlock, pflags);

//	spin_lock_irqsave(&ring->r_rxlock, pflags);
	/* clean rxblks ring */
	ringhd->rxblk_rdptr = ringhd->rxblk_wrptr;

//	spin_lock_irqsave(&ring->p_rxlock, qflags);
	/* recover rxblks pool */
	poolhd->rxblk_wrptr = poolhd->rxblk_rdptr;
	for (i = 0, j = 0; i < poolhd->rxblk_count; i++) {
		if (ring->rxrecord[i] == SBLOCK_BLK_STATE_DONE) {
			ring->p_rxblks[j].addr = i * sblock->rxblksz + poolhd->rxblk_addr;
			ring->p_rxblks[j].length = sblock->rxblksz;
			poolhd->rxblk_wrptr = poolhd->rxblk_wrptr + 1;
			j++;
		}
	}
//	spin_unlock_irqrestore(&ring->p_rxlock, qflags);
//	spin_unlock_irqrestore(&ring->r_rxlock, pflags);

	return 0;
}

static void sblock_thread(void *p1, void *p2, void *p3)
{
	struct sblock_mgr *sblock = (struct sblock_mgr*)p1;
	struct smsg mcmd, mrecv;
	int ret;
	int recovery = 0;
	//struct sblock blk;
	int prio;
	/*set the thread as a real time thread, and its priority is 90*/

	switch(sblock->channel) {
		case SMSG_CH_BT:
        case SMSG_CH_WIFI_CTRL:
            prio = QUEUE_PRIO_HIGH;
            break;
        default :
			prio = QUEUE_PRIO_NORMAL;
			break;
    }

	ret = smsg_ch_open(sblock->dst, sblock->channel,prio, 3000); 
	if (ret != 0) {
		ipc_error("Failed to open channel %d", sblock->channel);
		//return ret;
	}
    while(1)
    {
		ipc_debug("wait for channel %d data.", sblock->channel);
		/* monitor sblock recv smsg */
		smsg_set(&mrecv, sblock->channel, 0, 0, 0);
		ret = smsg_recv(sblock->dst, &mrecv, K_FOREVER);
		if (ret != 0) {
		/* channel state is FREE */
			k_sleep(100);
			continue;
		}
		ipc_debug("sblock thread recv msg: dst=%d, channel=%d, "
				"type=%d, flag=0x%04x, value=0x%08x",
				sblock->dst, sblock->channel,
				mrecv.type, mrecv.flag, mrecv.value);

		switch (mrecv.type) {
		case SMSG_TYPE_OPEN:
			/* handle channel recovery */
			if (recovery) {
				if (sblock->handler) {
/*这个中断处理是在main.c里面注册的block的中断处理 是通知应用处理sblock的消息的*/
					sblock->handler(SBLOCK_NOTIFY_CLOSE, sblock->data);
				}
				sblock_recover(sblock->dst, sblock->channel);
			}
			smsg_open_ack(sblock->dst, sblock->channel);
			break;
        case SMSG_TYPE_CLOSE:
			/* handle channel recovery */
			smsg_close_ack(sblock->dst, sblock->channel);
			if (sblock->handler) {
				sblock->handler(SBLOCK_NOTIFY_CLOSE, sblock->data);
			}
			sblock->state = SBLOCK_STATE_IDLE;
			break;
        case SMSG_TYPE_CMD:
			/* respond cmd done for sblock init */
			smsg_set(&mcmd, sblock->channel, SMSG_TYPE_DONE,
					SMSG_DONE_SBLOCK_INIT, sblock->smem_addr);
			smsg_send(sblock->dst,prio, &mcmd, -1);
			sblock->state = SBLOCK_STATE_READY;
			recovery = 1;
			ipc_debug("ap cp create %d channel success!",sblock->channel);
			if (sblock->channel == SMSG_CH_BT) {
                //sprd_bt_irq_init();
            }

/* 			sprd_wifi_send(sblock->channel,prio,cmd_hdr1,sizeof(cmd_hdr1));
			sleep(1);
			sprd_wifi_send(sblock->channel,prio,cmd_hdr1,sizeof(cmd_hdr1));
			sleep(1);
			sprd_wifi_send(sblock->channel,prio,cmd_hdr1,sizeof(cmd_hdr1));
			sleep(1);
			sprd_wifi_send(sblock->channel,prio,cmd_hdr1,sizeof(cmd_hdr1)); */
			break;
        case SMSG_TYPE_EVENT:
			/* handle sblock send/release events */
			//if (sblock->callback) {
			//	sblock->callback(sblock->channel, NULL, NULL);
			//}
#if 1
            switch (mrecv.flag) {
                case SMSG_EVENT_SBLOCK_SEND:
				//	ret = sblock_receive(0, sblock->channel, &blk, 0);
				//	recvdata = (uint8_t *)blk.addr;
				//	ipc_info("sblock recv 0x%x %x %x %x %x %x %x",recvdata[0],recvdata[1],recvdata[2],
				//	recvdata[3],recvdata[4],recvdata[5],recvdata[6]);
				//	if (ret != 0) {
				//		ipc_info("sblock recv error");
				//	}
                    if (sblock->callback) {
						sblock->callback(sblock->channel);
                    }
					//sblock_release(0,sblock->channel, &blk);
                    break;
				case SMSG_EVENT_SBLOCK_RECV:
					break;
                case SMSG_EVENT_SBLOCK_RELEASE:
				    //sblock_release(0,sblock->channel, &blk);
				  // wakeup_smsg_task_all(&(sblock->ring->getwait));
                    //if (sblock->handler) {
                      //  sblock->handler(SBLOCK_NOTIFY_GET, sblock->data);
                    //}
                    break;
                default:
                    ret = 1;
                    break;
            }
#endif
            break;
		default:
			ret = 1;
			break;
		};
		if (ret) {
			ipc_warn("non-handled sblock msg: %d-%d, %d, %d, %d",
					sblock->dst, sblock->channel,
					mrecv.type, mrecv.flag, mrecv.value);
			ret = 0;
		}
	}
    
}

int sblock_create(uint8_t dst, uint8_t channel,
		uint32_t txblocknum, uint32_t txblocksize,
		uint32_t rxblocknum, uint32_t rxblocksize)
{
	struct sblock_mgr *sblock;
	volatile struct sblock_ring_header *ringhd = NULL;
	volatile struct sblock_ring_header *poolhd = NULL;
	struct sblock_ring *ring = NULL;
	u32_t hsize;
	u32_t block_addr;
	int prio;
	int i;
	int ret;

	sblock = &sblocks[dst][channel];

	sblock->state = SBLOCK_STATE_IDLE;
	sblock->dst = dst;
	sblock->channel = channel;
	txblocksize = SBLOCKSZ_ALIGN(txblocksize,SBLOCK_ALIGN_BYTES);
	rxblocksize = SBLOCKSZ_ALIGN(rxblocksize,SBLOCK_ALIGN_BYTES);
	sblock->txblksz = txblocksize;
	sblock->rxblksz = rxblocksize;
	sblock->txblknum = txblocknum;
	sblock->rxblknum = rxblocknum;
    sblock->handler = NULL;
	sblock->callback = NULL;
	/* allocate smem */
	hsize = sizeof(struct sblock_header);
	sblock->smem_size = hsize +						/* for header*/
		txblocknum * txblocksize + rxblocknum * rxblocksize + 		/* for blks */
		(txblocknum + rxblocknum) * sizeof(struct sblock_blks) + 	/* for ring*/
		(txblocknum + rxblocknum) * sizeof(struct sblock_blks); 	/* for pool*/
	//这里分配共享内存 在NuttX里面sblock的共享内存怎么来确定
	switch(channel) {
		case SMSG_CH_WIFI_CTRL:
			block_addr = CTRLPATH_SBLOCK_SMEM_ADDR;
			break;
		case SMSG_CH_WIFI_DATA_NOR:
			block_addr = DATAPATH_SBLOCK_SMEM_ADDR;
			break;
		case SMSG_CH_WIFI_DATA_SPEC:
			block_addr = DATAPATH_SPEC_SBLOCK_SMEM_ADDR;
			break;
		case SMSG_CH_BT:
			block_addr = BT_SBLOCK_SMEM_ADDR;
			break;
		default :
			block_addr = CTRLPATH_SBLOCK_SMEM_ADDR;
			break;
	}
	sblock->smem_addr = block_addr;
	ipc_debug( "smem_addr 0x%x record_share_addr 0x%x",sblock->smem_addr,block_addr);

	ring = &sblock->ring;
	ring->header =(struct sblock_header *)sblock->smem_addr;
	ringhd = (volatile struct sblock_ring_header *)(&ring->header->ring);
	poolhd = (volatile struct sblock_ring_header *)(&ring->header->pool);

	ringhd->txblk_addr = sblock->smem_addr + hsize;
	ringhd->txblk_count = txblocknum;
	ringhd->txblk_size = txblocksize;
	ringhd->txblk_rdptr = 0;
	ringhd->txblk_wrptr = 0;
	ringhd->txblk_blks = sblock->smem_addr + hsize +
		txblocknum * txblocksize + rxblocknum * rxblocksize;  //这个地址是指向后面的 blk的管理block ring的地址
	ringhd->rxblk_addr = ringhd->txblk_addr + txblocknum * txblocksize;
	ringhd->rxblk_count = rxblocknum;
	ringhd->rxblk_size = rxblocksize;
	ringhd->rxblk_rdptr = 0;
	ringhd->rxblk_wrptr = 0;
	ringhd->rxblk_blks = ringhd->txblk_blks + txblocknum * sizeof(struct sblock_blks);

	poolhd->txblk_addr = sblock->smem_addr + hsize;
	poolhd->txblk_count = txblocknum;
	poolhd->txblk_size = txblocksize;
	poolhd->txblk_rdptr = 0;
	poolhd->txblk_wrptr = 0;
	poolhd->txblk_blks = ringhd->rxblk_blks + rxblocknum * sizeof(struct sblock_blks);
	poolhd->rxblk_addr = ringhd->txblk_addr + txblocknum * txblocksize;
	poolhd->rxblk_count = rxblocknum;
	poolhd->rxblk_size = rxblocksize;
	poolhd->rxblk_rdptr = 0;
	poolhd->rxblk_wrptr = 0;
	poolhd->rxblk_blks = poolhd->txblk_blks + txblocknum * sizeof(struct sblock_blks);
    ipc_debug("0x%p ringhd %p poolhd %p",sblock,ringhd,poolhd);

	//sblock->ring.header =(struct sblock_header	*)sblock->smem_addr;
   //这个地址实际上就是虚拟地址上的 ringhd->txblk_addr地址的对应值 完全对应了一套
	ring->r_txblks = (struct sblock_blks *)ringhd->txblk_blks;

	ring->r_rxblks = (struct sblock_blks *)ringhd->rxblk_blks;

	ring->p_txblks = (struct sblock_blks *)poolhd->txblk_blks;
	ring->p_rxblks = (struct sblock_blks *)poolhd->rxblk_blks;
	ring->txblk_virt = (void *)ringhd->txblk_addr;
	ring->rxblk_virt = (void *)ringhd->rxblk_addr;
/*这里是将每个blk的地址放到pool里面管理*/
	for (i = 0; i < txblocknum; i++) {
		ring->p_txblks[i].addr = poolhd->txblk_addr + i * txblocksize;
		ring->p_txblks[i].length = txblocksize;
		ring->txrecord[i] = SBLOCK_BLK_STATE_DONE;
		poolhd->txblk_wrptr++;
	}
	for (i = 0; i < rxblocknum; i++) {
		ring->p_rxblks[i].addr = poolhd->rxblk_addr + i * rxblocksize;
		ring->p_rxblks[i].length = rxblocksize;
		ring->rxrecord[i] = SBLOCK_BLK_STATE_DONE;
		poolhd->rxblk_wrptr++;
	}

    ring->yell = 1;
    k_sem_init(&ring->getwait, 0, 1);   
    k_sem_init(&ring->recvwait, 0, 1); 

	sblock->state = SBLOCK_STATE_READY;

	ipc_debug("r_txblks %p %p %p %p",ring->r_txblks,ring->r_rxblks,ring->p_txblks,ring->p_rxblks);

	
	sblock->pid = k_thread_create(&sblock->thread,
			sblock->sblock_stack, SBLOCK_STACK_SIZE,
			(k_thread_entry_t)sblock_thread, 
			(void *)sblock, NULL, NULL,
			 K_PRIO_COOP(7), 0, 0);
	if(sblock->pid == 0) {
		ipc_warn("Failed to create kthread: sblock-%d-%d", dst, channel);
		return -1;
	}

	ipc_error("sblock[%d] thread: 0x%x", channel, sblock->pid);

	return 0;
}

void sblock_destroy(uint8_t dst, uint8_t channel)
{
	struct sblock_mgr *sblock = &sblocks[dst][channel];
    int prio;

	switch(sblock->channel) {
        case SMSG_CH_WIFI_CTRL:
             prio = QUEUE_PRIO_HIGH;
             break;
        case SMSG_CH_WIFI_DATA_NOR:
             prio = QUEUE_PRIO_NORMAL;
            break;
        case SMSG_CH_WIFI_DATA_SPEC:
            prio = QUEUE_PRIO_NORMAL;
            break;
        default :
        prio = QUEUE_PRIO_NORMAL;
        break;
    }
    sblock->state = SBLOCK_STATE_IDLE;
	smsg_ch_close(dst, channel,prio, -1);

    	/* stop sblock thread if it's created successfully and still alive */
	if ((sblock->pid > 0)) {
		k_thread_abort(sblock->pid);
	}
#if 0
	if (sblock->ring) {
		wakeup_smsg_task_all(&sblock->ring->recvwait);
		wakeup_smsg_task_all(&sblock->ring->getwait);
		if (sblock->ring->txrecord) {
			free(sblock->ring->txrecord);
		}
		if (sblock->ring->rxrecord) {
			free(sblock->ring->rxrecord);
		}
		free(sblock->ring);
	}
#endif

    //smem_free(sblock->smem_addr, sblock->smem_size);

}

int sblock_register_callback(uint8_t channel,
		void (*callback)(int ch))
{
	int dst=0;
	struct sblock_mgr *sblock = &sblocks[dst][channel];
    ipc_debug("%d channel=%d %p", dst, channel,callback);

	sblock->callback = callback;

	return 0;
}

int sblock_state(u8_t channel)
{
	int dst = 0;
	struct sblock_mgr *sblock = &sblocks[dst][channel];

	return sblock->state;
}

int sblock_register_notifier(uint8_t dst, uint8_t channel,
		void (*handler)(int event, void *data), void *data)
{
	struct sblock_mgr *sblock = &sblocks[dst][channel];

#ifndef CONFIG_SIPC_WCN
	if (sblock->handler) {
		ipc_warn( "sblock handler already registered");
		return -EBUSY;
	}
#endif
	sblock->handler = handler;
	sblock->data = data;

	return 0;
}


/*这个地方是发送失败了 再重新放入pool*/
void sblock_put(uint8_t dst, uint8_t channel, struct sblock *blk)
{
	struct sblock_mgr *sblock = &sblocks[dst][channel];
	struct sblock_ring *ring = NULL;
	volatile struct sblock_ring_header *poolhd = NULL;
	//unsigned long flags;
	int txpos;
	int index;

    ring = &sblock->ring;
    poolhd = (volatile struct sblock_ring_header *)(&ring->header->pool);

    // spin_lock_irqsave(&ring->p_txlock, flags);
    txpos = sblock_get_ringpos(poolhd->txblk_rdptr-1,poolhd->txblk_count);
    ring->r_txblks[txpos].addr = (uint32_t)blk->addr;
    ring->r_txblks[txpos].length = poolhd->txblk_size;
    poolhd->txblk_rdptr = poolhd->txblk_rdptr - 1;
	ipc_debug("%d %d ",poolhd->txblk_rdptr,txpos);
    /*  在接收的那里需要从pool中获取blk的等待 这里进行了释放所以需要唤醒所有在这个信号量上等待的task*/
    if ((int)(poolhd->txblk_wrptr - poolhd->txblk_rdptr) == 1) {
		wakeup_smsg_task_all(&(ring->getwait));
	}

	index = sblock_get_index((blk->addr - ring->txblk_virt), sblock->txblksz);
	ring->txrecord[index] = SBLOCK_BLK_STATE_DONE;

	//spin_unlock_irqrestore(&ring->p_txlock, flags);
}

int sblock_get(uint8_t dst, uint8_t channel, struct sblock *blk, int timeout)
{
	struct sblock_mgr *sblock = &sblocks[dst][channel];
	struct sblock_ring *ring = NULL;
	//volatile struct sblock_ring_header *ringhd = NULL;
	volatile struct sblock_ring_header *poolhd = NULL;
	int txpos, index;
	int ret = 0;
	//unsigned long flags;

	ring = &sblock->ring;
	//ringhd = (volatile struct sblock_ring_header *)(&ring->header->ring);
	poolhd = (volatile struct sblock_ring_header *)(&ring->header->pool);
    ipc_debug("%d %d",poolhd->txblk_rdptr,poolhd->txblk_wrptr);

	if (poolhd->txblk_rdptr == poolhd->txblk_wrptr) {
		ret = k_sem_take(&ring->getwait, timeout);
		if (ret) {
			ipc_warn( "wait timeout!");
			ret = ret;
		}

		if(sblock->state == SBLOCK_STATE_IDLE) {
			ipc_warn( "sblock state is idle!");
			ret = -EIO;
		}
	}
	/* multi-gotter may cause got failure */
	//spin_lock_irqsave(&ring->p_txlock, flags);
	if (poolhd->txblk_rdptr != poolhd->txblk_wrptr &&
			sblock->state == SBLOCK_STATE_READY) {
		txpos = sblock_get_ringpos(poolhd->txblk_rdptr, poolhd->txblk_count);

		ipc_debug("txpos %d poolhd->txblk_rdptr %d",txpos,poolhd->txblk_rdptr);
		blk->addr = (void *)ring->p_txblks[txpos].addr;
		blk->length = poolhd->txblk_size;
		poolhd->txblk_rdptr++;
		index = sblock_get_index((blk->addr - ring->txblk_virt), sblock->txblksz);
		ring->txrecord[index] = SBLOCK_BLK_STATE_PENDING;
	} else {
		ret = sblock->state == SBLOCK_STATE_READY ? -EAGAIN : -EIO;
	}
	//spin_unlock_irqrestore(&ring->p_txlock, flags);

	return ret;
}

static int sblock_send_ex(uint8_t dst, uint8_t channel,uint8_t prio, struct sblock *blk, bool yell)
{
	struct sblock_mgr *sblock = &sblocks[dst][channel];
	struct sblock_ring *ring;
	volatile struct sblock_ring_header *ringhd;
	struct smsg mevt;
	int txpos, index;
	int ret = 0;
	//unsigned long flags;

	if (sblock->state != SBLOCK_STATE_READY) {
		ipc_warn("sblock-%d-%d not ready!", dst, channel);
		return sblock ? -EIO : -ENODEV;
	}

	ipc_info("dst=%d, channel=%d, addr=%p, len=%d",
			dst, channel, blk->addr, blk->length);

    ring = &sblock->ring;
	ringhd = (volatile struct sblock_ring_header *)(&ring->header->ring);

	//spin_lock_irqsave(&ring->r_txlock, flags);
	txpos = sblock_get_ringpos(ringhd->txblk_wrptr, ringhd->txblk_count);
	ring->r_txblks[txpos].addr = (uint32_t)blk->addr;
	ring->r_txblks[txpos].length = blk->length;
    ringhd->txblk_wrptr++;

	ipc_debug("txpos: %d, wrptr: 0x%x", txpos, ringhd->txblk_wrptr);

	//extern void read8_cmd_exe(u32_t addr, u32_t len);
	//read8_cmd_exe((u32_t)blk->addr, blk->length + BLOCK_HEADROOM_SIZE);

	if (sblock->state == SBLOCK_STATE_READY) {
		if(yell) {
			smsg_set(&mevt, channel, SMSG_TYPE_EVENT, SMSG_EVENT_SBLOCK_SEND, 0);
			ret = smsg_send(dst,prio, &mevt, 0);
		}
		else if(!ring->yell) {
			if(((int)(ringhd->txblk_wrptr - ringhd->txblk_rdptr) == 1) /*&&
																		 ((int)(poolhd->txblk_wrptr - poolhd->txblk_rdptr) == (sblock->txblknum - 1))*/) {
				ring->yell = 1;
			}
		}
	}

	index = sblock_get_index((blk->addr - ring->txblk_virt), sblock->txblksz);
 	ring->txrecord[index] = SBLOCK_BLK_STATE_DONE;

	//spin_unlock_irqrestore(&ring->r_txlock, flags);

	return ret ;
}

int sblock_send(uint8_t dst, uint8_t channel,uint8_t prio, struct sblock *blk)
{
	return sblock_send_ex(dst, channel, prio, blk, true);
}

int sblock_send_prepare(uint8_t dst, uint8_t channel,uint8_t prio, struct sblock *blk)
{
	return sblock_send_ex(dst, channel,prio, blk, false);
}

int sblock_send_finish(uint8_t dst, uint8_t channel)
{
	struct sblock_mgr *sblock = &sblocks[dst][channel];
	struct sblock_ring *ring;
	//volatile struct sblock_ring_header *ringhd;
	struct smsg mevt;
	int ret=0;

	if (sblock->state != SBLOCK_STATE_READY) {
		ipc_warn("sblock-%d-%d not ready!", dst, channel);
		return sblock ? -EIO : -ENODEV;
	}

	ring = &sblock->ring;
	//ringhd = (volatile struct sblock_ring_header *)(&ring->header->ring);

	if (ring->yell) {
                ring->yell = 0;
		smsg_set(&mevt, channel, SMSG_TYPE_EVENT, SMSG_EVENT_SBLOCK_SEND, 0);
		ret = smsg_send(dst,QUEUE_PRIO_NORMAL, &mevt, 0);
	}

	return ret;
}

int sblock_receive(uint8_t dst, uint8_t channel, struct sblock *blk, int timeout)
{
	struct sblock_mgr *sblock = &sblocks[dst][channel];
	struct sblock_ring *ring;
	volatile struct sblock_ring_header *ringhd;
	int rxpos, index, ret = 0;
	//unsigned long flags;

	if (sblock->state != SBLOCK_STATE_READY) {
		ipc_warn("sblock-%d-%d not ready!", dst, channel);
		return sblock ? -EIO : -ENODEV;
	}
	ring = &sblock->ring;
	ringhd = (volatile struct sblock_ring_header *)(&ring->header->ring);

	ipc_debug("sblock_receive: dst=%d, channel=%d, timeout=%d",dst, channel, timeout);
	ipc_debug("sblock_receive: channel=%d, wrptr=%d, rdptr=%d",channel, ringhd->rxblk_wrptr, ringhd->rxblk_rdptr);

	if (ringhd->rxblk_wrptr == ringhd->rxblk_rdptr) {
		if (timeout == 0) {
			/* no wait */
			ipc_info("sblock_receive %d-%d is empty!",dst, channel);
			ret = -ENODATA;
		} else if (timeout < 0) {
			/* wait forever */
			ret = k_sem_take(&ring->recvwait, K_FOREVER);
			if (ret < 0) {
				ipc_warn( "sblock_receive wait interrupted!");
			}

			if (sblock->state == SBLOCK_STATE_IDLE) {
				ipc_warn( "sblock_receive sblock state is idle!");
				ret = -EIO;
			}

		} else {
			/* wait timeout */
			ret = k_sem_take(&ring->recvwait,timeout);
			if (ret < 0) {
				ipc_warn( "sblock_receive wait interrupted!");
			} else if (ret == 0) {
				ipc_warn( "sblock_receive wait timeout!");
				ret = -ETIME;
			}

			if (sblock->state == SBLOCK_STATE_IDLE) {
				ipc_warn( "sblock_receive sblock state is idle!");
				ret = -EIO;
			}
		}
	}

	if (ret < 0) {
		return ret;
	}

	/* multi-receiver may cause recv failure */
	//spin_lock_irqsave(&ring->r_rxlock, flags);

	if (ringhd->rxblk_wrptr != ringhd->rxblk_rdptr &&
			sblock->state == SBLOCK_STATE_READY) {
		rxpos = sblock_get_ringpos(ringhd->rxblk_rdptr, ringhd->rxblk_count);
		blk->addr = (void *)ring->r_rxblks[rxpos].addr;
		blk->length = ring->r_rxblks[rxpos].length;
		ringhd->rxblk_rdptr = ringhd->rxblk_rdptr + 1;
		ipc_debug("sblock_receive: channel=%d, rxpos=%d, addr=%p, len=%d",
			channel, rxpos, blk->addr, blk->length);
		index = sblock_get_index((blk->addr - ring->rxblk_virt), sblock->rxblksz);
		ring->rxrecord[index] = SBLOCK_BLK_STATE_PENDING;
	} else {
		ret = sblock->state == SBLOCK_STATE_READY ? -EAGAIN : -EIO;
	}
	//spin_unlock_irqrestore(&ring->r_rxlock, flags);
	return ret;
}

int sblock_get_arrived_count(uint8_t dst, uint8_t channel)
{
	struct sblock_mgr *sblock = &sblocks[dst][channel];
	struct sblock_ring *ring = NULL;
	volatile struct sblock_ring_header *ringhd = NULL;
	int blk_count = 0;
	//unsigned long flags;

	if (sblock->state != SBLOCK_STATE_READY) {
		ipc_warn("sblock-%d-%d not ready!", dst, channel);
		return -ENODEV;
	}

	ring = &sblock->ring;
	ringhd = (volatile struct sblock_ring_header *)(&ring->header->ring);

	//spin_lock_irqsave(&ring->r_rxlock, flags);
	blk_count = (int)(ringhd->rxblk_wrptr - ringhd->rxblk_rdptr);
	//spin_unlock_irqrestore(&ring->r_rxlock, flags);

	return blk_count;

}

int sblock_get_free_count(uint8_t dst, uint8_t channel)
{
	struct sblock_mgr *sblock = &sblocks[dst][channel];
	struct sblock_ring *ring = NULL;
	volatile struct sblock_ring_header *poolhd = NULL;
	int blk_count = 0;


	if (sblock->state != SBLOCK_STATE_READY) {
		ipc_warn( "sblock-%d-%d not ready!", dst, channel);
		return -ENODEV;
	}

	ring = &sblock->ring;
	poolhd = (volatile struct sblock_ring_header *)(&ring->header->pool);

	//spin_lock_irqsave(&ring->p_txlock, flags);
	blk_count = (int)(poolhd->txblk_wrptr - poolhd->txblk_rdptr);
	//spin_unlock_irqrestore(&ring->p_txlock, flags);

	return blk_count;
}


int sblock_release(uint8_t dst, uint8_t channel, struct sblock *blk)
{
	struct sblock_mgr *sblock = &sblocks[dst][channel];
	struct sblock_ring *ring = NULL;
	//volatile struct sblock_ring_header *ringhd = NULL;
	volatile struct sblock_ring_header *poolhd = NULL;
	struct smsg mevt;
	int rxpos;
	int index;

	if (sblock->state != SBLOCK_STATE_READY) {
		ipc_warn( "sblock-%d-%d not ready!", dst, channel);
		return -ENODEV;
	}

	ipc_debug("sblock_release: dst=%d, channel=%d, addr=%p, len=%d",
			dst, channel, blk->addr, blk->length);

	ring = &sblock->ring;
	//ringhd = (volatile struct sblock_ring_header *)(&ring->header->ring);
	poolhd = (volatile struct sblock_ring_header *)(&ring->header->pool);

	//spin_lock_irqsave(&ring->p_rxlock, flags);
	rxpos = sblock_get_ringpos(poolhd->rxblk_wrptr, poolhd->rxblk_count);
	ring->p_rxblks[rxpos].addr = (uint32_t)blk->addr;
	ring->p_rxblks[rxpos].length = poolhd->rxblk_size;
	poolhd->rxblk_wrptr = poolhd->rxblk_wrptr + 1;
	ipc_debug("sblock_release: addr=%x", ring->p_rxblks[rxpos].addr);

	if((int)(poolhd->rxblk_wrptr - poolhd->rxblk_rdptr) == 1 &&
			sblock->state == SBLOCK_STATE_READY) {
		/* send smsg to notify the peer side */
		smsg_set(&mevt, channel, SMSG_TYPE_EVENT, SMSG_EVENT_SBLOCK_RELEASE, 0);
		smsg_send(dst,QUEUE_PRIO_NORMAL, &mevt, -1);
	}

	index = sblock_get_index((blk->addr - ring->rxblk_virt), sblock->rxblksz);
	ring->rxrecord[index] = SBLOCK_BLK_STATE_DONE;

	//spin_unlock_irqrestore(&ring->p_rxlock, flags);

	return 0;
}
