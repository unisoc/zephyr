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

#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN	"UNISOC"

#define GPIO_PORT0		"UWP_GPIO_P0"
#define UART_2			"UART_2"
#define UWP_WDG		CONFIG_WDT_UWP_DEVICE_NAME

#define GPIO0		0
#define GPIO2		2

#define ON		1
#define OFF		0

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
	struct net_if *iface;

	SYS_LOG_WRN("Run dhcpv4 client");

	net_mgmt_init_event_callback(&mgmt_cb, handler,
				     NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&mgmt_cb);

	iface = net_if_get_default();

	net_dhcpv4_start(iface);

	return 0;
}
void led_switch(struct device *dev)

{
	static int sw = ON;

	sw = (sw == ON) ? OFF : ON;
	gpio_pin_write(dev, GPIO2, sw);
}

#if 0
struct gpio_callback cb;

static void gpio_callback(struct device *dev,
		struct gpio_callback *gpio_cb, u32_t pins)
{
	SYS_LOG_INF("main gpio int.\n");
}

void timer_expiry(struct k_timer *timer)
{
	static int c = 0;
	SYS_LOG_INF("timer_expiry %d.", c++);
}

void timer_stop(struct k_timer *timer)
{
	static int c = 0;
	SYS_LOG_INF("timer_stop %d.", c++);
}
#endif

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

/* WDT Requires a callback, there is no interrupt enable / disable. */
void wdt_example_cb(struct device *dev, int channel_id)
{

}

void wdg_init(void)
{
	struct wdt_timeout_cfg wdt_cfg;
	struct wdt_config wr_cfg;
	struct wdt_config cfg;
	struct device *wdt_dev;

	wdt_cfg.callback = wdt_example_cb;
	wdt_cfg.flags = WDT_FLAG_RESET_SOC;
	wdt_cfg.window.min = 0;
	wdt_cfg.window.max = 4000;

	wr_cfg.timeout = 4000;
	wr_cfg.mode = WDT_MODE_INTERRUPT_RESET;
	wr_cfg.interrupt_fn = wdt_example_cb;

	wdt_dev = device_get_binding(UWP_WDG);
	if(wdt_dev == NULL) {
		SYS_LOG_ERR("Can not find device %s.", UWP_WDG);
		return;
	}

#if 0
	ret = wdt_install_timeout(wdt_dev, &wdt_cfg);
	if (ret < 0) {
		SYS_LOG_ERR("wdt install error.");
		return;
	}

	ret = wdt_setup(wdt_dev, 0);
	if (ret < 0) {
		SYS_LOG_ERR("Watchdog setup error\n");
		return;
	}
#endif
	wdt_set_config(wdt_dev, &wr_cfg);
	wdt_enable(wdt_dev);


	wdt_get_config(wdt_dev, &cfg);
}

#if 0
//#define FLASH_TEST_REGION_OFFSET 0xff000
#define FLASH_TEST_REGION_OFFSET 0x0
#define FLASH_SECTOR_SIZE        4096
#define TEST_DATA_BYTE_0         0x55
#define TEST_DATA_BYTE_1         0xaa
#define TEST_DATA_LEN            0x100
void spi_flash_test(void)
{
	struct device *flash_dev;
	u8_t buf[TEST_DATA_LEN];
	u8_t *p = 0x100000;

	printk("\nW25QXXDV SPI flash testing\n");
	printk("==========================\n");
	printk("flash name:%s.\n",CONFIG_FLASH_UWP_NAME);

	flash_dev = device_get_binding(CONFIG_FLASH_UWP_NAME);

	if (!flash_dev) {
		printk("SPI flash driver was not found!\n");
		return;
	}

	/* Write protection needs to be disabled in w25qxxdv flash before
	 * each write or erase. This is because the flash component turns
	 * on write protection automatically after completion of write and
	 * erase operations.
	 */
#if 1
	printk("\nTest 1: Flash erase\n");
	flash_write_protection_set(flash_dev, false);
	if (flash_erase(flash_dev,
				FLASH_TEST_REGION_OFFSET,
				FLASH_SECTOR_SIZE) != 0) {
		printk("   Flash erase failed!\n");
	} else {
		printk("   Flash erase succeeded!\n");
	}

	printk("\nTest 2: Flash write\n");
	flash_write_protection_set(flash_dev, false);

	buf[0] = TEST_DATA_BYTE_0;
	buf[1] = TEST_DATA_BYTE_1;
	if (flash_write(flash_dev, FLASH_TEST_REGION_OFFSET, p,
				TEST_DATA_LEN) != 0) {
		printk("   Flash write failed!\n");
		return;
	}
#endif

	if (flash_read(flash_dev, FLASH_TEST_REGION_OFFSET, buf,
				TEST_DATA_LEN) != 0) {
		printk("   Flash read failed!\n");
		return;
	}

	if(memcmp(p, buf, TEST_DATA_LEN) == 0) {
		printk("   Data read matches with data written. Good!!\n");
	} else {
		printk("   Data read does not match with data written!!\n");
	}

}

#endif

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
#if 0
	while(i--) {
		uart_poll_out(uart, 'a');
	}
#endif

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

#if 0
extern int iwnpi_main(int argc, char **argv);
static int iwnpi_cmd(int argc, char **argv)
{
	iwnpi_main(argc, argv);
	return 0;
}
/* NFFS work area strcut */
static struct nffs_flash_desc flash_desc;

/* mounting info */
static struct fs_mount_t nffs_mnt = {
	.type = FS_NFFS,
	.mnt_point = "/zephyr",
	.fs_data = &flash_desc,
};

static int test_mount(void)
{
	struct device *flash_dev;
	int res;

	flash_dev = device_get_binding(CONFIG_FS_NFFS_FLASH_DEV_NAME);
	if (!flash_dev) {
		printk("get flash %s failed.\n", CONFIG_FS_NFFS_FLASH_DEV_NAME);
		return -ENODEV;
	}
	printk("get flash %s success.\n", CONFIG_FS_NFFS_FLASH_DEV_NAME);

	/* set backend storage dev */
	nffs_mnt.storage_dev = flash_dev;

	res = fs_mount(&nffs_mnt);
	if (res < 0) {
		printk("Error mounting nffs [%d]\n", res);
		return -1;
	}

	printk("mount success.\n");

	return 0;
}

static int flash_cmd(int argc, char **argv)
{
	int ret;

	if(!strcmp(argv[1], "mount")) {
		ret = test_mount();
	}
	return 0;
}
#endif

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

void gpio_init(void)
{
	struct device *gpio;

	gpio = device_get_binding(GPIO_PORT0);
	if(gpio == NULL) {
		SYS_LOG_ERR("Can not find device %s.", GPIO_PORT0);
		return;
	}

	gpio_pin_configure(gpio, GPIO2, GPIO_DIR_OUT
			| GPIO_PUD_PULL_DOWN);

#if 0
	gpio_pin_disable_callback(gpio, GPIO0);

	gpio_pin_configure(gpio, GPIO0,
			GPIO_DIR_IN | GPIO_INT | GPIO_INT_LEVEL | \
			GPIO_INT_ACTIVE_LOW | GPIO_PUD_PULL_UP);

	gpio_init_callback(&cb, gpio_callback, BIT(GPIO0));
	gpio_add_callback(gpio, &cb);

	gpio_pin_enable_callback(gpio, GPIO0);
#endif
}

#if 1
#define FATFS_MNTP "/NAND:"
/* FatFs work area */
static FATFS fat_fs;

/* mounting info */
static struct fs_mount_t fatfs_mnt = {
	.type = FS_FATFS,
	.mnt_point = FATFS_MNTP,
	.fs_data = &fat_fs,
};

void download_wifi_ini(void)
{
	int ret;
	ret = fs_mount(&fatfs_mnt);
	if (ret < 0) {
		SYS_LOG_ERR("Error mounting fs [%d]\n", ret);
		return;
	}

	ret = uwp_wifi_download_ini();
	if (ret) {
		SYS_LOG_ERR("Download wifi ini failed.");
		return;
	}
}
#endif

void main(void)
{
	//int ret;
	//struct k_timer timer;
	SYS_LOG_WRN("Unisoc Wi-Fi Repeater.");

	SHELL_REGISTER("zephyr", zephyr_cmds);

	//shell_register_default_module("zephyr");

	gpio_init();
	wdg_init();

	download_wifi_ini();

	while(1) {}
	//uwp_aon_irq_enable(AON_INT_GPIO0);

#if 0
	k_timer_init(&timer, timer_expiry, timer_stop);
	k_timer_user_data_set(&timer, (void *)portf);
	SYS_LOG_INF("start timer..\n");
	k_timer_start(&timer, K_SECONDS(1), K_SECONDS(5));
#endif

	while(1) {}
}
