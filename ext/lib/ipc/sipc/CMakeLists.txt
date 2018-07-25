zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_SIPC	smsg.c)
zephyr_sources_ifdef(CONFIG_SIPC	sblock.c)
zephyr_sources_ifdef(CONFIG_SIPC	sipc.c)
