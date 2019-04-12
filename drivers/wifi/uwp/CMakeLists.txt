if(CONFIG_WIFI_UWP)
  zephyr_include_directories(./)

  zephyr_sources(
	cmdevt.c
	txrx.c
	ipc.c
	main.c
	mem.c
    )
endif()
