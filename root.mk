################################################################
# Root Makefile of the whole project
################################################################

# Basic Definitions
################################################################
ARCH		:= arm
ABI		:= eabi
BOARD		?= uwp566x_evb
BOOT		:= mcuboot
KERNEL		:= zephyr

TARGET		:= $(ARCH)-zephyr-$(ABI)
CROSS_COMPILE	:= $(TARGET)-

# Directories and Files
################################################################
SHELL		:= /bin/bash
PWD		:= $(shell pwd)
PRJDIR		:= $(PWD)
HWCONFIG_DIR	:= $(PRJDIR)/hwconfig

app_DIR		:= $(PRJDIR)/app
boot_DIR	:= $(PRJDIR)/$(BOOT)
fdl_DIR		:= $(PRJDIR)/u-boot
kernel_DIR	:= $(PRJDIR)/$(KERNEL)
toolchain_DIR	:= $(PRJDIR)/toolchain
dloader_DIR	:= $(toolchain_DIR)/LinuxDloader

BUILD_DIR		:= $(PRJDIR)/build
boot_BUILD_DIR		:= $(BUILD_DIR)/$(BOOT)
fdl_BUILD_DIR		:= $(fdl_DIR)
kernel_BUILD_DIR	:= $(BUILD_DIR)/$(KERNEL)
boot_debug_BUILD_DIR	:= $(BUILD_DIR)/$(BOOT)_debug
kernel_debug_BUILD_DIR	:= $(BUILD_DIR)/$(KERNEL)_debug

BOOT_BIN	:= $(boot_BUILD_DIR)/$(KERNEL)/$(KERNEL).bin
KERNEL_BIN	:= $(kernel_BUILD_DIR)/$(KERNEL)/$(KERNEL).bin
FDL_BIN		:= $(fdl_DIR)/u-boot.bin
DLOADER_BIN	:= $(dloader_DIR)/Bin/sprd_dloader/dloader

DIST_DIR	:= $(PRJDIR)/build/images
BOOT_DIST_BIN	:= $(DIST_DIR)/$(BOOT)-pubkey.bin
KERNEL_DIST_BIN	:= $(DIST_DIR)/$(KERNEL)-signed-ota.bin
FDL_DIST_BIN	:= $(DIST_DIR)/fdl.bin
CONFIG_DIST_BIN	:= $(DIST_DIR)/config.fat

cmake_FILE	:= $(toolchain_DIR)/cmake-3.8.2-Linux-x86_64.sh
cmake_DIR	:= $(basename $(cmake_FILE))
export PATH	:= $(cmake_DIR)/bin:$(PATH)

sdk_FILE	:= $(toolchain_DIR)/zephyr-sdk-0.9.3-setup.run
sdk_DIR		:= $(basename $(sdk_FILE))
export PATH	:= $(sdk_DIR)/sysroots/x86_64-pokysdk-linux/usr/bin/$(TARGET):$(PATH)

export ZEPHYR_TOOLCHAIN_VARIANT	:= zephyr
export ZEPHYR_SDK_INSTALL_DIR	:= $(sdk_DIR)

IMGTOOL = $(boot_DIR)/scripts/imgtool.py
ASSEMBLE = $(boot_DIR)/scripts/assemble.py

# Macros
################################################################
# MESSAGE Macro -- display a message in bold type
MESSAGE = echo "\n$(TERM_BOLD)>>> $(1)$(TERM_RESET)"
TERM_BOLD := $(shell tput smso 2>/dev/null)
TERM_RESET := $(shell tput rmso 2>/dev/null)

# Macro of Extracting cmake
# $(1): cmake file
# $(2): cmake directory
define EXTRACT_cmake
	cd $(toolchain_DIR); \
	yes | sh $(1) | cat; \
	cmake --version; \
	touch $(2);
endef

# Macro of Extracting Zephyr SDK
# $(1): SDK file
# $(2): SDK directory
define EXTRACT_sdk
	sh $(1) -- -d $(2); \
	touch $(2);
endef

# Macro of Preparing Building Environment
# $(1): Target
define MAKE_ENV
.PHONY: $(1)
$(1):
	@ if [ ! -d $($(1)_DIR) ]; then \
		$(call MESSAGE,"Preparing $(1)"); \
		$(call EXTRACT_$(1),$($(1)_FILE),$($(1)_DIR)) \
	fi
	@ if ( find $($(1)_FILE) -newer $($(1)_DIR) | grep -q $($(1)_FILE) ); then \
		$(call MESSAGE,"Updating $(1)"); \
		rm -rf $($(1)_DIR); \
		$(call EXTRACT_$(1),$($(1)_FILE),$($(1)_DIR)) \
	fi
endef

# Macro of Building Targets
# $(1): Target
# $(2): Dir of main
define MAKE_TARGET
.PHONY: $(1)
$(1): $(ENV_TARGETS)
	@ $(call MESSAGE,"Building $(1)")
	@ if [ ! -d $($(1)_BUILD_DIR) ]; then mkdir -p $($(1)_BUILD_DIR); fi
	(source $(kernel_DIR)/zephyr-env.sh && cd $($(1)_BUILD_DIR) && \
	if [ ! -f Makefile ] ; then cmake -DBOARD=$(BOARD) -DCONF_FILE=prj$(findstring _debug,$(1)).conf $(2); fi && \
	make \
	)
endef

# Macro of Cleaning Targets
# $(1): Target
# $(2): Target suffix
# $(3): .config
define CLEAN_TARGET
.PHONY: $(if $(2),$(1)-$(2),$(1))
$(if $(2),$(1)-$(2),$(1)):
	@ $(call MESSAGE,"Cleaning $(1)")
	@ if [ -d $($(1)_BUILD_DIR) ]; then make -C $($(1)_BUILD_DIR) $(2); fi
endef

SIGNING_KEY	?= $(boot_DIR)/root-rsa-2048.pem
BOOT_HEADER_LEN	:= 0x200
FLASH_ALIGNMENT	:= 8

# Macro of Signing KERNEL Image
# $(1): input file
# $(2): output file
define SIGN_KERNEL_IMAGE
	@ $(IMGTOOL) sign \
		--key $(SIGNING_KEY) \
		--header-size $(BOOT_HEADER_LEN) \
		--align $(FLASH_ALIGNMENT) \
		--version 1.2 \
		--included-header \
		$(1) $(2)
endef

# Macro of Generating Config Image
# $(1): Path of config image
# $(n): Name of config files
define GEN_CONFIG_IMAGE
	@ rm -f $(1)
	@ mkfs.vfat -a -S 4096 -v -C $(1) 128
	@ (cd $(HWCONFIG_DIR) && \
	if [ -f $(2) ]; then mcopy -i $(1) $(2) ::; fi && \
	if [ -f $(3) ]; then mcopy -i $(1) $(3) ::; fi && \
	if [ -f $(4) ]; then mcopy -i $(1) $(4) ::; fi && \
	if [ -f $(5) ]; then mcopy -i $(1) $(5) ::; fi)
endef

# Targets
################################################################
ENV_TARGETS		:= cmake sdk
DEFAULT_TARGETS		:= boot kernel
DIST_TARGETS		:= $(DEFAULT_TARGETS) fdl dloader
DEBUG_TARGETS		:= $(addsuffix _debug,$(DEFAULT_TARGETS))
ALL_TARGETS		:= $(DEFAULT_TARGETS) $(DEBUG_TARGETS)
CLEAN_TARGETS		:= $(addsuffix -clean,$(ALL_TARGETS))

uboot_DIR	:= $(PRJDIR)/u-boot
KEY_DIR		:= $(uboot_DIR)/rsa-keypair
KEY_NAME	:= dev
KEY_DTB		:= $(uboot_DIR)/u-boot-pubkey.dtb
ITB		:= $(uboot_DIR)/fit-$(BOARD).itb

FLASH_BASE		:= 0x02000000
KERNEL_PARTTION_OFFSET	:= 0x00040000
KERNEL_BOOT_ADDR	:= 0x$(shell printf "%08x" $(shell echo $$(( $(FLASH_BASE) + $(KERNEL_PARTTION_OFFSET) ))))
FIT_HEADER_SIZE		:= 0x1000
KERNEL_LOAD_ADDR	:= 0x$(shell printf "%08x" $(shell echo $$(( $(KERNEL_BOOT_ADDR) + $(FIT_HEADER_SIZE) ))))
KERNEL_ENTRY_ADDR	:= 0x$(shell printf "%08x" $(shell echo $$(( $(KERNEL_LOAD_ADDR) + 0x4 ))))

# Macro of Signing OS Image
# $(1): Compression type
# $(2): Load address
# $(3): Entry point
# $(4): Key dir
# $(5): Key name
define SIGN_OS_IMAGE
	ITS=$(uboot_DIR)/fit-$(BOARD).its; \
	cp -f $(uboot_DIR)/dts/dt.dtb $(KEY_DTB); \
	$(uboot_DIR)/scripts/mkits.sh \
		-D $(BOARD) -o $$ITS -k $(KERNEL_BIN) -C $(1) -a $(2) -e $(3) -A $(ARCH) -K $(KERNEL) $(if $(5),-s $(5)); \
	PATH=$(uboot_DIR)/tools:$(uboot_DIR)/scripts/dtc:$(PATH) mkimage -f $$ITS -K $(KEY_DTB) $(if $(4),-k $(4)) -r -E -p $(FIT_HEADER_SIZE) $(ITB)
endef

.PHONY: dist
dist: $(DIST_TARGETS)
	@ if [ ! -d $(DIST_DIR) ]; then install -d $(DIST_DIR); fi
	@ install $(FDL_BIN) $(FDL_DIST_BIN)
	@ install $(BOOT_BIN) $(BOOT_DIST_BIN)
	$(call SIGN_KERNEL_IMAGE,$(KERNEL_BIN),$(KERNEL_DIST_BIN))
	$(call GEN_CONFIG_IMAGE,$(CONFIG_DIST_BIN),wifi_board_config.ini,bt_configure_pskey.ini,bt_configure_rf.ini,bt_info.ini)
#	building u-boot temporarily
	@ if [ ! -f $(DIST_DIR)/u-boot-pubkey-dtb.bin ]; then \
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(uboot_DIR) distclean; \
	sed -i 's/bootm 0x......../bootm $(KERNEL_BOOT_ADDR)/' $(uboot_DIR)/include/configs/$(BOARD).h; \
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(uboot_DIR) $(BOARD)_defconfig; \
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(uboot_DIR); \
	fi
#	sign kernel for u-boot loading
	@ mv $(KERNEL_BIN) $(KERNEL_BIN).orig
	@ dd if=$(KERNEL_BIN).orig of=$(KERNEL_BIN) bs=4K skip=1
	$(call SIGN_OS_IMAGE,none,$(KERNEL_LOAD_ADDR),$(KERNEL_ENTRY_ADDR),$(KEY_DIR),$(KEY_NAME))
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(uboot_DIR) EXT_DTB=$(KEY_DTB)
	@ mv $(KERNEL_BIN).orig $(KERNEL_BIN)
	install $(ITB) $(DIST_DIR)/zephyr-signed-ota.bin
	install $(uboot_DIR)/u-boot.bin $(DIST_DIR)/u-boot-pubkey-dtb.bin
	install $(DIST_DIR)/u-boot-pubkey-dtb.bin $(DIST_DIR)/mcuboot-pubkey.bin

.PHONY: debug
debug: $(DEBUG_TARGETS)

#@killall JLinkGDBServer || true
#JLinkGDBServer -device Cortex-M4 -endian little -if SWD -speed 8000 -jlinkscriptfile /opt/SEGGER/JLink/Samples/JLink/Scripts/SPRD_SC2355.JLinkScript &
#${ZEPHYR_SDK_INSTALL_DIR}/sysroots/x86_64-pokysdk-linux/usr/bin/arm-zephyr-eabi/arm-zephyr-eabi-gdb $(kernel_BUILD_DIR)/zephyr/zephyr.elf

.PHONY: clean
clean: $(CLEAN_TARGETS)

.PHONY: distclean
distclean:
	@ if [ -d $(BUILD_DIR) ]; then rm -rf $(BUILD_DIR); fi
	@ $(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(fdl_DIR) $@

# Respective Targets
################################################################
# Environment Targets
$(foreach target,$(ENV_TARGETS),$(eval $(call MAKE_ENV,$(target))))

# Build Targets
$(eval $(call MAKE_TARGET,boot,$(boot_DIR)/boot/zephyr))

$(eval $(call MAKE_TARGET,kernel,$(kernel_DIR)/samples/repeater))

$(eval $(call MAKE_TARGET,boot_debug,$(boot_DIR)/boot/zephyr))

$(eval $(call MAKE_TARGET,kernel_debug,$(kernel_DIR)/samples/repeater))

$(FDL_BIN):
	@ $(call MESSAGE,"Building fdl")
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(fdl_DIR) $(BOARD)_fdl_defconfig
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(fdl_DIR)

.PHONY: fdl
fdl: $(ENV_TARGETS) $(FDL_BIN)

$(DLOADER_BIN):
	@ $(call MESSAGE,"Building dloader")
	$(MAKE) -C $(dloader_DIR)/Source

.PHONY: dloader
dloader: $(DLOADER_BIN)

# Clean Targets
$(foreach target,$(ALL_TARGETS),$(eval $(call CLEAN_TARGET,$(target),clean)))
