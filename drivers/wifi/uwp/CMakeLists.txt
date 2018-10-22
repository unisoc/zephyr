if(CONFIG_WIFI_UWP)
  zephyr_include_directories(./)

  zephyr_sources(
	wifi_cmdevt.c
	wifi_txrx.c
	wifi_ipc.c
	wifi_main.c
    )
endif()
