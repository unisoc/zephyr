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

app_DIR		:= $(PRJDIR)/app
boot_DIR	:= $(PRJDIR)/$(BOOT)
fdl_DIR		:= $(PRJDIR)/u-boot
kernel_DIR	:= $(PRJDIR)/$(KERNEL)
toolchain_DIR	:= $(PRJDIR)/toolchain

BUILD_DIR		:= $(PRJDIR)/build
boot_BUILD_DIR		:= $(BUILD_DIR)/$(BOOT)
fdl_BUILD_DIR		:= $(fdl_DIR)
kernel_BUILD_DIR	:= $(BUILD_DIR)/$(KERNEL)
boot_debug_BUILD_DIR	:= $(BUILD_DIR)/$(BOOT)_debug
kernel_debug_BUILD_DIR	:= $(BUILD_DIR)/$(KERNEL)_debug

BOOT_BIN	:= $(boot_BUILD_DIR)/$(KERNEL)/$(KERNEL).bin
KERNEL_BIN	:= $(kernel_BUILD_DIR)/$(KERNEL)/$(KERNEL).bin
FDL_BIN		:= $(fdl_DIR)/u-boot.bin

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
# $(2): Target suffix
# $(3): Dir of main
define MAKE_TARGET
.PHONY: $(if $(2),$(1)-$(2),$(1))
$(if $(2),$(1)-$(2),$(1)): $(ENV_TARGETS)
	@ $(call MESSAGE,"Building $(1)")
	@ if [ ! -d $($(if $(2),$(1)_$(2),$(1))_BUILD_DIR) ]; then mkdir -p $($(if $(2),$(1)_$(2),$(1))_BUILD_DIR); fi
	(source $(kernel_DIR)/zephyr-env.sh && cd $($(if $(2),$(1)_$(2),$(1))_BUILD_DIR) && \
	if [ ! -f Makefile ] ; then cmake -DBOARD=$(BOARD) $(if $(2),,-DCONF_FILE=prj_xip.conf) $(3); fi && \
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
	$(IMGTOOL) sign \
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
	if [ -f $(4) ]; then mcopy -i $(1) $(4) ::; fi)
endef

# Targets
################################################################
ENV_TARGETS		:= cmake sdk
DEFAULT_TARGETS		:= boot kernel fdl
DEBUG_TARGETS		:= boot-debug kernel-debug
ALL_TARGETS		:= $(DEFAULT_TARGETS) $(DEBUG_TARGETS)
CLEAN_TARGETS		:= $(addsuffix -clean,$(ALL_TARGETS))

.PHONY: all
all: $(DEFAULT_TARGETS)
	@ if [ ! -d $(DIST_DIR) ]; then install -d $(DIST_DIR); fi
	install $(FDL_BIN) $(FDL_DIST_BIN)
	install $(BOOT_BIN) $(BOOT_DIST_BIN)
	$(call SIGN_KERNEL_IMAGE,$(KERNEL_BIN),$(KERNEL_DIST_BIN))
	$(call GEN_CONFIG_IMAGE,$(CONFIG_DIST_BIN),wifi_board_config.ini,bt_configure_pskey.ini,bt_configure_rf.ini)

.PHONY: debug
debug: $(DEBUG_TARGETS)
	@killall JLinkGDBServer || true
	JLinkGDBServer -device Cortex-M4 -endian little -if SWD -speed 8000 -jlinkscriptfile /opt/SEGGER/JLink/Samples/JLink/Scripts/SPRD_SC2355.JLinkScript &
	${ZEPHYR_SDK_INSTALL_DIR}/sysroots/x86_64-pokysdk-linux/usr/bin/arm-zephyr-eabi/arm-zephyr-eabi-gdb $(kernel_BUILD_DIR)/zephyr/zephyr.elf

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
$(eval $(call MAKE_TARGET,boot,,$(boot_DIR)/boot/zephyr))

$(eval $(call MAKE_TARGET,kernel,,$(kernel_DIR)/samples/repeater))

$(eval $(call MAKE_TARGET,boot,debug,$(boot_DIR)/boot/zephyr))

$(eval $(call MAKE_TARGET,kernel,debug,$(kernel_DIR)/samples/repeater))

.PHONY: fdl
fdl:
	@ $(call MESSAGE,"Building fdl")
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $($@_DIR) $(BOARD)_fdl_defconfig
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $($@_DIR)

# Clean Targets
$(foreach target,$(ALL_TARGETS),$(eval $(call CLEAN_TARGET,$(target),clean)))
