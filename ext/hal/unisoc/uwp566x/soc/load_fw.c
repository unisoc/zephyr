/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <logging/sys_log.h>
#include <uwp_hal.h>

#define CONFIG_CP_SECTOR1_LOAD_BASE 0x40a20000
#define CONFIG_CP_SECTOR2_LOAD_BASE 0x40a80000
#define CONFIG_CP_SECTOR3_LOAD_BASE 0x40e40000
#define CONFIG_CP_SECTOR4_LOAD_BASE 0x40f40000
#define CONFIG_CP_SECTOR1_LEN 360448
#define CONFIG_CP_SECTOR2_LEN 196608
#define CONFIG_CP_SECTOR3_LEN 131072
#define CONFIG_CP_SECTOR4_LEN 249856


#define CP_START_ADDR (0x020C0000 + 16)
#define CP_RUNNING_CHECK_CR 0x40a80000
#define CP_RUNNING_BIT	0
#define CP_WIFI_RUNNING_BIT	1


extern void GNSS_Start(void);

int move_cp(char *src,char *dst,uint32_t size)
{
	int *from,*to;
	int len =0;
	char *from_8,*to_8;

	if(src == NULL || dst == NULL ||size == 0){
		printk("invalid parameter,src=%p,dst=%p,size=%d\n",src,dst,size);
		return -1;
	}else {
		printk("copy %u byte from %p to %p\n",size,src,dst);
	}
	from = (int *)src;
	to = (int *)dst;
	len =size/4;
	while(len > 0){
		*to = *from;
		to ++;
		from ++;
		len --;
	}
	len = size %4;
	from_8 = (char *)from;
	to_8 = (char *)to;
	while(len > 0){
		*to_8 = *from_8;
		to_8++;
		from_8++;
		len --;
	}
	printk("sector load done,from =%p to =%p\n",from_8,to_8);
	return 0;	
}

int load_fw(void)
{
	int ret  = 0;
	char *src = NULL;
	uint32_t offset = 0;
		
	printk("load cp firmware start\n");
	//load sector1
	src = (char *)(CP_START_ADDR);
	ret = move_cp(src,(char *)CONFIG_CP_SECTOR1_LOAD_BASE,(uint32_t)CONFIG_CP_SECTOR1_LEN);
	offset +=CONFIG_CP_SECTOR1_LEN;
	//load sector 2
	src = (char *)(CP_START_ADDR +offset);
	ret =move_cp(src,(char *)CONFIG_CP_SECTOR2_LOAD_BASE,(uint32_t)CONFIG_CP_SECTOR2_LEN);
	offset +=CONFIG_CP_SECTOR2_LEN;
	//load sector 3
	src = (char *)(CP_START_ADDR +offset);
	move_cp(src,(char *)CONFIG_CP_SECTOR3_LOAD_BASE,(uint32_t)CONFIG_CP_SECTOR3_LEN);
	offset +=CONFIG_CP_SECTOR3_LEN;
	//load sector 4
	src = (char *)(CP_START_ADDR +offset);
	//move_cp(src,(char *)CONFIG_CP_SECTOR4_LOAD_BASE,(uint32_t)CONFIG_CP_SECTOR4_LEN);
	
	if (ret < 0){
		printk("something wrong,move cp return -1\n");
		return -1;
	}else{
		printk("cp firmware copy done\n");
	}

	return 0;
}

int cp_mcu_pull_reset(void)
{
	int i = 0;

	printk("gnss mcu hold start\n");
	//dap sel
	sci_reg_or(0x4083c064, BIT(0) | BIT(1));
	//dap rst
	sci_reg_and(0x4083c000, ~(BIT(28) | BIT(29)));
	//dap eb
	sci_reg_or(0x4083c024, BIT(30) | BIT(31));
	//check dap is ok ?
	while ( sci_read32(0x408600fc) != 0x24770011 && i < 20){
		i++;
	}
	if (sci_read32(0x408600fc) != 0x24770011){
		printk("check dap is ok fail\n");
	}
	// hold gnss core
	sci_write32(0x40860000, 0x22000012);
	sci_write32(0x40860004, 0xe000edf0);
	sci_write32(0x4086000c, 0xa05f0003);
	//restore dap sel
	sci_reg_and(0x4083c064, ~(BIT(0) | BIT(1)));
	//remap gnss RAM to 0x0000_0000 address as boot from RAM
	sci_reg_or(0x40bc800c, BIT(0) | BIT(1));

	printk("gnss mcu hold done\n");

	return 0;
}

int cp_mcu_release_reset(void)
{
	unsigned int value =0;

	printk("gnss mcu release start. \n");
	// reset the gnss CM4 core,and CM4 will run from IRAM(which is remapped to 0x0000_0000)
	value = sci_read32(0x40bc8004);
	value |=0x1;
	sci_write32(0x40bc8004, value);

	printk("gnss mcu release done. \n");

	return 0;
}
void  cp_check_bit_clear(void)
{
	sci_write32(CP_RUNNING_CHECK_CR, 0);
	printk("cp running check bit cleared\n");
	
	return ;
}

int cp_check_running(void)
{
	int value;
	int cnt = 100;

	printk("check if cp is running\n");
	
	do{
		value = sci_read32(CP_RUNNING_CHECK_CR);
		if (value &(1 << CP_RUNNING_BIT) ){
			printk("CP FW is running !!!\n");
			return 0;
		}
		k_sleep(60);
	}while(cnt-- > 0);

	printk("CP FW running fail,Something must be wrong\n");
	return -1;
}

int cp_check_wifi_running(void)
{
	int value;
	int cnt = 100;

	printk("check if cp wifi is running\n");
	do{
		value = sci_read32(CP_RUNNING_CHECK_CR);
		if (value &(1 << CP_WIFI_RUNNING_BIT) ){
			printk("CP wifi is running !!! %d\n", cnt);

			while(1) ;
			return 0;
		}
	}while(cnt-- > 0);

	printk("CP wifi running fail,Something must be wrong\n");
	return -1;
}



int cp_mcu_init(void)
{
	int ret = 0;

	SYS_LOG_INF("aon eb: 0x%x.", sci_read32(REG_AON_GLB_EB));
	GNSS_Start();
	ret = cp_mcu_pull_reset();
	if (ret <0 ){
		printk("reset CP MCU fail\n");
		return -1;
	}
	ret = load_fw();
	if (ret < 0 ){
		printk("load fw fail\n");
		return -1 ;
	}
	cp_check_bit_clear();
	ret = cp_mcu_release_reset();//cp start to run
	if (ret < 0){
		printk("release reset fail\n");
		return -1;
	}

	ret = cp_check_running();
	if (ret < 0){
		printk("cp fw is not running,something must be wrong\n");
		return -1;
	}
	SYS_LOG_INF("aon eb: 0x%x.", sci_read32(REG_AON_GLB_EB));

	printk("CP Init done,and CP fw is running!!!\n");
	return 0;
}
