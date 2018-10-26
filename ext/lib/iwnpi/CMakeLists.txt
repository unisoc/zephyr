zephyr_include_directories(.)
zephyr_sources_ifdef(CONFIG_IWNPI cmd2355.c)
zephyr_sources_ifdef(CONFIG_IWNPI wlnpi.c)
zephyr_sources_ifdef(CONFIG_IWNPI wifi_npi.c)
