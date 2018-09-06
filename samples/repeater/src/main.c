/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>

#include <gpio.h>
#include <watchdog.h>
#include <device.h>
#include <console.h>
#include <flash.h>
#include <shell/shell.h>
#include <string.h>
#include <misc/util.h>
#include <fs.h>
#include <uart.h>
#include <stdlib.h>
#include <logging/sys_log.h>

#include <fs.h>
#include <ff.h>

#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>

#include "uwp_hal.h"
#include "bluetooth/blues.h"

#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN	"UNISOC"

#define UART_2			"UART_2"

typedef void (*uwp_intc_callback_t) (int channel, void *user);
extern void uwp_intc_set_irq_callback(int channel,
		uwp_intc_callback_t cb, void *arg);
extern void uwp_intc_unset_irq_callback(int channel);
extern void uwp_aon_intc_set_irq_callback(int channel,
		uwp_intc_callback_t cb, void *arg);
extern void uwp_aon_intc_unset_irq_callback(int channel);

void device_list_get(struct device **device_list, int *device_count);

static struct net_mgmt_event_callback mgmt_cb;

static void handler(struct net_mgmt_event_callback *cb,
		    u32_t mgmt_event,
		    struct net_if *iface)
{
	int i = 0;

	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		return;
	}

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		char buf[NET_IPV4_ADDR_LEN];

		if (iface->config.ip.ipv4->unicast[i].addr_type !=
							NET_ADDR_DHCP) {
			continue;
		}

		SYS_LOG_WRN("Your address: %s",
			 net_addr_ntop(AF_INET,
			    &iface->config.ip.ipv4->unicast[i].address.in_addr,
				       buf, sizeof(buf)));
		SYS_LOG_WRN("Lease time: %u seconds",
			 iface->config.dhcpv4.lease_time);
		SYS_LOG_WRN("Subnet: %s",
			 net_addr_ntop(AF_INET,
				       &iface->config.ip.ipv4->netmask,
				       buf, sizeof(buf)));
		SYS_LOG_WRN("Router: %s",
			 net_addr_ntop(AF_INET, &iface->config.ip.ipv4->gw,
				       buf, sizeof(buf)));
	}
}

int dhcp_client(int argc, char **argv)
{
	struct device *dev;
	struct net_if *iface;

	SYS_LOG_WRN("Run dhcpv4 client");

	net_mgmt_init_event_callback(&mgmt_cb, handler,
				     NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&mgmt_cb);

	dev = device_get_binding(CONFIG_WIFI_UWP_STA_NAME);
	if (!dev) {
		SYS_LOG_ERR( "failed to get device %s!\n", CONFIG_WIFI_UWP_STA_NAME);
		return -1;
	}

	iface = net_if_lookup_by_dev(dev);
	if (!iface) {
		SYS_LOG_ERR("failed to get iface %s!\n", CONFIG_WIFI_UWP_STA_NAME);
		return -1;
	}

	net_dhcpv4_start(iface);

	return 0;
}

void print_devices(void)
{
	int i;
	int count = 0;

	static struct device *device_list;

	device_list_get(&device_list, &count);

	SYS_LOG_INF("device list(%d):", count);
	for(i = 0; i < count; i++) {
		SYS_LOG_INF(" %s ", device_list[i].config->name);
	}
}

void uart_test(void)
{
	struct device *uart;
	char *str = "This is a test message from test command.\r\n";

	uart = device_get_binding(UART_2);
	if(uart == NULL) {
		SYS_LOG_ERR("Can not find device %s.", UART_2);
		return;
	}

	uart_irq_rx_enable(uart);

	uart_fifo_fill(uart, str, strlen(str));

	SYS_LOG_INF("test uart %s finish.", UART_2);
}

static void intc_uwp_soft_irq(int channel, void *data)
{
	if(data) {
		SYS_LOG_INF("aon soft irq.");
		uwp_aon_irq_clear_soft();
	} else {
		SYS_LOG_INF("soft irq.");
		uwp_irq_clear_soft();
	}
}

static int test_cmd(int argc, char *argv[])
{
	if(argc < 2) return -EINVAL;

	if(!strcmp(argv[1], "flash")) {
		//spi_flash_test();
	}else if(!strcmp(argv[1], "uart")) {
		uart_test();
	}else if(!strcmp(argv[1], "intc")) {
		uwp_intc_set_irq_callback(INT_SOFT, intc_uwp_soft_irq, (void*)0);
		uwp_irq_enable(INT_SOFT);

		uwp_irq_trigger_soft();

		uwp_irq_disable(INT_SOFT);
		uwp_intc_unset_irq_callback(INT_SOFT);
	}else if(!strcmp(argv[1], "aon_intc")) {
		uwp_aon_intc_set_irq_callback(INT_SOFT, intc_uwp_soft_irq, (void*)1);
		uwp_aon_irq_enable(INT_SOFT);

		uwp_aon_irq_trigger_soft();

		uwp_aon_irq_disable(INT_SOFT);
		uwp_aon_intc_unset_irq_callback(INT_SOFT);
	}else {
		return -EINVAL;
	}

	return 0;
}

static int info_cmd(int argc, char *argv[])
{
	if(argc != 1) return -EINVAL;

	SYS_LOG_INF("System Info: \n");

	print_devices();

	return 0;
}

static int sleep_cmd(int argc, char *argv[])
{
	int time;

	if(argc != 2) return -EINVAL;

	time = atoi(argv[1]);
	SYS_LOG_INF("sleep %ims start...", time);

	k_sleep(time);

	SYS_LOG_INF("sleep stop.");

	return 0;
}

static int dev_cmd(int argc, char **argv)
{
	struct device *devices;
	int count;
	int i;

	device_list_get(&devices, &count);

	SYS_LOG_WRN("System device lists:");
	for (i = 0; i < count; i++, devices++) {
		if(strcmp(devices->config->name, "") == 0)
			continue;
		SYS_LOG_WRN("%d: %s.", i, devices->config->name);
	}

	return 0;
}

u32_t str2hex(char *s)
{
	u32_t val = 0;
	int len;
	int i;
	char c;

	if (strncmp(s, "0x", 2) && strncmp(s, "0X", 2)) {
		SYS_LOG_ERR("Invalid hex string: %s.", s);
		return 0;
	}

	/* skip 0x */
	s += 2;

	len = strlen(s);
	if(len > 8) {
		SYS_LOG_ERR("Invalid hex string.");
		return 0;
	}

	for (i = 0; i < len; i++) {
		c = *s;

		/* 0 - 9 */
		if(c >= '0' && c <= '9')
			c -= '0';
		else if (c >= 'A' && c <= 'F') 
			c = c - 'A' + 10;
		else if (c >= 'a' && c <= 'f')
			c = c - 'a' + 10;
		else {
			SYS_LOG_ERR("Invalid hex string.");
			return 0;
		}

		val = val * 16 + c;

		s++;
	}

	return val;
}

u32_t str2data(char *s)
{
	if (strncmp(s, "0x", 2) == 0 || strncmp(s, "0X", 2) == 0)
		return str2hex(s);
	else
		return atoi(s);
}

void read8_cmd_exe(u32_t addr, u32_t len)
{
	int i;
	SYS_LOG_WRN("Read %d bytes from 0x%x:", len, addr);

	for(i = 0; i < len; i++, addr++) {
		if(i%16 == 0) printk("\n%08x: ", addr );
		printk("%02x ", sys_read8(addr));
	}
	printk("\n");
}

static int read8_cmd(int argc, char **argv)
{
	u32_t addr;
	u32_t len;
	if(argc != 3) return -EINVAL;

	addr = (u32_t)str2data(argv[1]);
	len = str2data(argv[2]);

	read8_cmd_exe(addr, len);

	return 0;
}
void read32_cmd_exe(u32_t addr, u32_t len)
{
	int i;
	SYS_LOG_WRN("Read data from 0x%x:", addr);

	for(i = 0; i < len; i++, addr += 4) {
		if(i%4 == 0) printk("\n%08x: ", addr );
		printk("%08x ", sys_read32(addr));
	}
	printk("\n");
}

static int read32_cmd(int argc, char **argv)
{
	u32_t addr;
	u32_t len;
	if(argc != 3) return -EINVAL;

	addr = (u32_t)str2data(argv[1]);
	len = str2data(argv[2]);

	read32_cmd_exe(addr, len);

	return 0;
}

static int write_cmd(int argc, char **argv)
{
	u32_t p;
	u32_t val;
	if(argc != 3) return -EINVAL;

	p = str2data(argv[1]);
	val = str2data(argv[2]);

	sys_write32(val, p);

	SYS_LOG_INF("Write data 0x%x to address 0x%x success.",
			val, p);

	return 0;
}

extern int dhcp_client(int argc, char **argv);

static const struct shell_cmd zephyr_cmds[] = {
	{ "info", info_cmd, "" },
	{ "sleep", sleep_cmd, "time(ms)" },
	{ "test", test_cmd, "<flash|uart|intc|aon_intc>" },
	{ "device", dev_cmd, "" },
	{ "read8", read8_cmd, "adress len" },
	{ "read32", read32_cmd, "adress len" },
	{ "write32", write_cmd, "adress value" },
	{ "dhcp_client", dhcp_client, "" },
	{ NULL, NULL }
	//{ "iwnpi", iwnpi_cmd, "adress value" },
	//	{ "flash", flash_cmd, "mount|read|write address value" },
};

#define FATFS_MNTP "/NAND:"
/* FatFs work area */
static FATFS fat_fs;

/* mounting info */
static struct fs_mount_t fatfs_mnt = {
	.type = FS_FATFS,
	.mnt_point = FATFS_MNTP,
	.fs_data = &fat_fs,
};

void mount_file_system(void)
{
	int ret;
	ret = fs_mount(&fatfs_mnt);
	if (ret < 0) {
		SYS_LOG_ERR("Error mounting fs [%d]\n", ret);
	}
}

void main(void)
{
	SYS_LOG_WRN("Unisoc Wi-Fi Repeater.");

	SHELL_REGISTER("zephyr", zephyr_cmds);

	mount_file_system();

	blues_init();

	while(1) {}
}
