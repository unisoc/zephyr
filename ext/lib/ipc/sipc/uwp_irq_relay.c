/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <string.h>
#include <logging/sys_log.h>
#include <uwp_hal.h>
#include "sipc.h"
#include "sipc_priv.h"

#define NVIC_AON_INT_IRQ_REQ_BB_TS   0xb

#if defined(CONFIG_BT_CTLR_WORKER_PRIO)
#define RADIO_TICKER_USER_ID_WORKER_PRIO CONFIG_BT_CTLR_WORKER_PRIO
#else
#define RADIO_TICKER_USER_ID_WORKER_PRIO 0
#endif

#if defined(CONFIG_BT_CTLR_JOB_PRIO)
#define RADIO_TICKER_USER_ID_JOB_PRIO CONFIG_BT_CTLR_JOB_PRIO
#else
#define RADIO_TICKER_USER_ID_JOB_PRIO 0
#endif

#define CTL_INTC_BASE            0x40000000

#define CEVA_IP_INT_CLEAR_ADDR (0x40246000+0x28)
#define CEVA_IP_int_clear(clear_bit) {int temp=*((volatile uint32_t *)CEVA_IP_INT_CLEAR_ADDR);temp |=clear_bit;*((volatile uint32_t *)CEVA_IP_INT_CLEAR_ADDR)=temp;}

#define CEVA_IP_int_MASK(bit) {int temp=*((volatile uint32_t *)CEVA_IP_INT_CLEAR_ADDR);temp |=bit;*((volatile uint32_t *)CEVA_IP_INT_CLEAR_ADDR)=temp;}
#define CEVA_IP_int_UNMASK(bit) {int temp=*((volatile uint32_t *)CEVA_IP_INT_CLEAR_ADDR);temp &=~bit;*((volatile uint32_t *)CEVA_IP_INT_CLEAR_ADDR)=temp;}


#define HW_DEC_INT_CLEAR (0x40240000+0x304)
#define HW_DEC_INT1_CLEAR (0x40240000+0x308)

#define HW_DEC_int_clear(clear_bit) \
{unsigned int temp = *((volatile uint32_t *)HW_DEC_INT_CLEAR); \
 temp |= clear_bit; \
 *((volatile uint32_t*)HW_DEC_INT_CLEAR)=temp; \
 temp &= ~clear_bit; \
 *((volatile uint32_t*)HW_DEC_INT_CLEAR)=temp; }

#define HW_DEC_int_clear_sts \
{unsigned int temp1; \
unsigned int temp= *((volatile uint32_t *)HW_DEC_INT_CLEAR); \
 temp1 = (temp >>16)&0xff; \
 *((volatile uint32_t*)HW_DEC_INT_CLEAR)=temp | temp1; \
 *((volatile uint32_t*)HW_DEC_INT_CLEAR)=temp&(~0xff); }

#define HW_DEC_int1_clear_sts \
{unsigned int temp1; \
unsigned int temp= *((volatile uint32_t *)HW_DEC_INT1_CLEAR); \
 temp1 = (temp >>16)&0xff; \
 *((volatile uint32_t*)HW_DEC_INT1_CLEAR)=temp | temp1; \
 *((volatile uint32_t*)HW_DEC_INT1_CLEAR)=temp&(~0xff); }

#define  PKD_INTR_MASK          (1<<4)
#define  AUX_TMR_INTR_MASK      (1<<5)
#define  PKA_INTR_MASK          (1<<6)
#define  PKD_RX_HDR_INTR_MASK   (1<<7)
#define  PKD_NO_PKD_INTR_MASK   (1<<13)
#define  SYNC_DET_INTR_MASK     (1<<14)


#define TIM_INTRO_CLR     (1<<16)
#define TIM_INTR1_CLR     (1<<17)
#define TIM_INTR2_CLR     (1<<18)
#define TIM_INTR3_CLR     (1<<19)
#define AUX_TMR_INTR      (1<<21)
#define PKA_INTR          (1<<22)
#define SYNC_DET_INTR     (1<<30)
#define PKD_RX_HDR        (1<<23)
#define PKD_INTR          (1<<20)
#define PAGE_TIMEOUT_INTR (1<<29)
#define ATOR_INTR0        (1<<0)
#define ATOR_INTR1        (1<<1)
#define ATOR_INTR2        (1<<2)


void clear_bt_int(int irq_num)
{
	switch (irq_num){
		case NVIC_BT_MASKED_TIM_INTR0 :  CEVA_IP_int_clear(TIM_INTRO_CLR) ; break;
		case NVIC_BT_MASKED_TIM_INTR1 :  CEVA_IP_int_clear(TIM_INTR1_CLR) ; break;
		case NVIC_BT_MASKED_TIM_INTR2 :  CEVA_IP_int_clear(TIM_INTR2_CLR) ; break;
		case NVIC_BT_MASKED_TIM_INTR3 :  CEVA_IP_int_clear(TIM_INTR3_CLR) ; break;
		case NVIC_BT_MASKED_AUX_TMR_INTR : CEVA_IP_int_clear(AUX_TMR_INTR) ; break;
		case NVIC_BT_MASKED_PKA_INTR :    CEVA_IP_int_MASK(PKA_INTR_MASK); break;
 		case NVIC_BT_MASKED_SYNC_DET_INTR : CEVA_IP_int_MASK(SYNC_DET_INTR_MASK); break;
		case NVIC_BT_MASKED_PKD_RX_HDR :  CEVA_IP_int_MASK(PKD_RX_HDR_INTR_MASK); break;
		case NVIC_BT_MASKED_PKD_INTR :    CEVA_IP_int_MASK(PKD_INTR_MASK); break;
		case NVIC_BT_MASKED_PAGE_TIMEOUT_INTR : CEVA_IP_int_MASK(PKD_NO_PKD_INTR_MASK);  break;
		case NVIC_BT_ACCELERATOR_INTR0 :  HW_DEC_int_clear(ATOR_INTR0); break;
		case NVIC_BT_ACCELERATOR_INTR1 :  HW_DEC_int_clear(ATOR_INTR1); break;
		case NVIC_BT_ACCELERATOR_INTR2 :  HW_DEC_int_clear(ATOR_INTR2); break;
		case NVIC_BT_ACCELERATOR_INTR3 :  HW_DEC_int_clear_sts; break;
		case NVIC_BT_ACCELERATOR_INTR4 :  HW_DEC_int1_clear_sts; break;

		default :
		     ipc_info("bt clear irq error %d\n",irq_num);
		break;
	}

}
void sprd_bt_irq_enable(void)
{

    irq_enable(NVIC_BT_MASKED_PAGE_TIMEOUT_INTR);
    irq_enable(NVIC_BT_MASKED_SYNC_DET_INTR);
    irq_enable(NVIC_BT_MASKED_PKD_RX_HDR);
    irq_enable(NVIC_BT_MASKED_TIM_INTR0);
    irq_enable(NVIC_BT_MASKED_TIM_INTR1);
    irq_enable(NVIC_BT_MASKED_TIM_INTR2);
    irq_enable(NVIC_BT_MASKED_TIM_INTR3);
    irq_enable(NVIC_BT_MASKED_PKD_INTR);
    irq_enable(NVIC_BT_MASKED_PKA_INTR);
    irq_enable(NVIC_BT_MASKED_AUX_TMR_INTR);
    irq_enable(NVIC_BT_ACCELERATOR_INTR0);
    irq_enable(NVIC_BT_ACCELERATOR_INTR1);
    irq_enable(NVIC_BT_ACCELERATOR_INTR2);
    irq_enable(NVIC_BT_ACCELERATOR_INTR3);
    irq_enable(NVIC_BT_ACCELERATOR_INTR4);

}
void sprd_bt_irq_disable(void)
{

	irq_disable(NVIC_BT_MASKED_PAGE_TIMEOUT_INTR);
	irq_disable(NVIC_BT_MASKED_SYNC_DET_INTR);
	irq_disable(NVIC_BT_MASKED_PKD_RX_HDR);
	irq_disable(NVIC_BT_MASKED_TIM_INTR0);
	irq_disable(NVIC_BT_MASKED_TIM_INTR1);
	irq_disable(NVIC_BT_MASKED_TIM_INTR2);
	irq_disable(NVIC_BT_MASKED_TIM_INTR3);
	irq_disable(NVIC_BT_MASKED_PKD_INTR);
	irq_disable(NVIC_BT_MASKED_PKA_INTR);
	irq_disable(NVIC_BT_MASKED_AUX_TMR_INTR);
	irq_disable(NVIC_BT_ACCELERATOR_INTR0);
	irq_disable(NVIC_BT_ACCELERATOR_INTR1);
	irq_disable(NVIC_BT_ACCELERATOR_INTR2);
	irq_disable(NVIC_BT_ACCELERATOR_INTR3);
	irq_disable(NVIC_BT_ACCELERATOR_INTR4);

}

static int bt_irq_handler(void *arg)
{
    struct smsg msg;
    int32_t irq = (int32_t)arg;

    clear_bt_int(irq);
    smsg_set(&msg, SMSG_CH_IRQ_DIS, SMSG_TYPE_EVENT, 0, irq);
    smsg_send_irq(SIPC_ID_AP,&msg);
    return 0;
}

int sprd_bt_irq_init(void)
{

	IRQ_CONNECT(NVIC_BT_MASKED_PAGE_TIMEOUT_INTR, RADIO_TICKER_USER_ID_WORKER_PRIO,bt_irq_handler,
				NVIC_BT_MASKED_PAGE_TIMEOUT_INTR, 0);
	IRQ_CONNECT(NVIC_BT_MASKED_SYNC_DET_INTR, RADIO_TICKER_USER_ID_WORKER_PRIO,bt_irq_handler,
				NVIC_BT_MASKED_SYNC_DET_INTR, 0);
	IRQ_CONNECT(NVIC_BT_MASKED_PKD_RX_HDR, RADIO_TICKER_USER_ID_WORKER_PRIO,bt_irq_handler,
				NVIC_BT_MASKED_PKD_RX_HDR, 0);
	IRQ_CONNECT(NVIC_BT_MASKED_TIM_INTR0, RADIO_TICKER_USER_ID_WORKER_PRIO,bt_irq_handler,
				NVIC_BT_MASKED_TIM_INTR0, 0);
    IRQ_CONNECT(NVIC_BT_MASKED_TIM_INTR1, RADIO_TICKER_USER_ID_WORKER_PRIO,bt_irq_handler,
				NVIC_BT_MASKED_TIM_INTR1, 0);
	IRQ_CONNECT(NVIC_BT_MASKED_TIM_INTR2, RADIO_TICKER_USER_ID_WORKER_PRIO,bt_irq_handler,
				NVIC_BT_MASKED_TIM_INTR2, 0);
	IRQ_CONNECT(NVIC_BT_MASKED_TIM_INTR3, RADIO_TICKER_USER_ID_WORKER_PRIO,bt_irq_handler,
				NVIC_BT_MASKED_TIM_INTR3, 0);
	IRQ_CONNECT(NVIC_BT_MASKED_PKD_INTR, RADIO_TICKER_USER_ID_WORKER_PRIO,bt_irq_handler,
				NVIC_BT_MASKED_PKD_INTR, 0);
	IRQ_CONNECT(NVIC_BT_MASKED_PKA_INTR, RADIO_TICKER_USER_ID_WORKER_PRIO,bt_irq_handler,
				NVIC_BT_MASKED_PKA_INTR, 0);
	IRQ_CONNECT(NVIC_BT_MASKED_AUX_TMR_INTR, RADIO_TICKER_USER_ID_WORKER_PRIO,bt_irq_handler,
				NVIC_BT_MASKED_AUX_TMR_INTR, 0);
	IRQ_CONNECT(NVIC_BT_ACCELERATOR_INTR0, RADIO_TICKER_USER_ID_WORKER_PRIO,bt_irq_handler,
				NVIC_BT_ACCELERATOR_INTR0, 0);
 	IRQ_CONNECT(NVIC_BT_ACCELERATOR_INTR1, RADIO_TICKER_USER_ID_WORKER_PRIO,bt_irq_handler,
				NVIC_BT_ACCELERATOR_INTR1, 0);
	IRQ_CONNECT(NVIC_BT_ACCELERATOR_INTR2, RADIO_TICKER_USER_ID_WORKER_PRIO,bt_irq_handler,
				NVIC_BT_ACCELERATOR_INTR2, 0);
	IRQ_CONNECT(NVIC_BT_ACCELERATOR_INTR3, RADIO_TICKER_USER_ID_WORKER_PRIO,bt_irq_handler,
				NVIC_BT_ACCELERATOR_INTR3, 0);
	IRQ_CONNECT(NVIC_BT_ACCELERATOR_INTR4, RADIO_TICKER_USER_ID_WORKER_PRIO,bt_irq_handler,
				NVIC_BT_ACCELERATOR_INTR4, 0);

    sprd_bt_irq_enable();

    return 0;
}

/*wifi irq regster enable request dis*/
void sprd_wifi_irq_enable(void)
{

    irq_enable(NVIC_INT_MAC);
    irq_enable(NVIC_INT_REQ_WIFI_CAP);
    irq_enable(NVIC_INT_DPD);
	irq_enable(NVIC_INT_REQ_COM_TMR);

}
void sprd_wifi_irq_disable(void)
{
    irq_disable(NVIC_INT_MAC);
    irq_disable(NVIC_INT_REQ_WIFI_CAP);
    irq_disable(NVIC_INT_DPD);
    irq_disable(NVIC_INT_REQ_COM_TMR);

}
void sprd_wifi_irq_enable_num(uint32_t num)
{
	ipc_info("wifi irq enable %d\n",num);

	switch (num) {
		case NVIC_INT_MAC:
			irq_enable(NVIC_INT_MAC);
		break;
		case NVIC_INT_REQ_COM_TMR:
			irq_enable(NVIC_INT_REQ_COM_TMR);
		break;
		case NVIC_INT_REQ_WIFI_CAP:
			irq_enable(NVIC_INT_REQ_WIFI_CAP);
		break;
		case NVIC_INT_DPD:
			irq_enable(NVIC_INT_DPD);
		break;
		default :
			ipc_info("wifi irq enable error num %d\n",num);
		break;
	}

}
void sprd_wifi_irq_disable_num(uint32_t num)
{
	ipc_info("wifi irq enable %d\n",num);
	switch (num) {
		case NVIC_INT_MAC:
			irq_disable(NVIC_INT_MAC);
		break;
		case NVIC_INT_REQ_COM_TMR:
			irq_disable(NVIC_INT_REQ_COM_TMR);
		break;
		case NVIC_INT_REQ_WIFI_CAP:
			irq_disable(NVIC_INT_REQ_WIFI_CAP);
		break;
		case NVIC_INT_DPD:
			irq_disable(NVIC_INT_DPD);
		break;
		default :
			ipc_info("wifi irq disable error num %d\n",num);
		break;
	}
    //irq_disable(NVIC_INT_REQ_COM_TMR);


}

static int wifi_int_irq_handler(void *arg)
{
    struct smsg msg;
    int32_t irq = (int32_t)arg;
    //ipc_info("wifi irq %d\n",irq);

    smsg_set(&msg, SMSG_CH_IRQ_DIS, SMSG_TYPE_EVENT, 0, irq);
    smsg_send_irq(SIPC_ID_AP,&msg);

    return 0;
}

int wifi_irq_init(void)
{

	IRQ_CONNECT(NVIC_INT_MAC, 5,wifi_int_irq_handler,
				NVIC_INT_MAC, 0);
	IRQ_CONNECT(NVIC_INT_REQ_WIFI_CAP, 5,wifi_int_irq_handler,
				NVIC_INT_REQ_WIFI_CAP, 0);
	IRQ_CONNECT(NVIC_INT_DPD, 5,wifi_int_irq_handler,
				NVIC_INT_DPD, 0);
	IRQ_CONNECT(NVIC_INT_REQ_COM_TMR, 5,wifi_int_irq_handler,
				NVIC_INT_REQ_COM_TMR, 0);

    return 0;
}
