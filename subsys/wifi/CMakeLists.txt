zephyr_compile_options(
  )

zephyr_sources_ifdef(
  CONFIG_WIFIMGR
  wifimgr.c
  sm.c
  cmd_prcs.c
  evt_lsnr.c
  sta.c
  ap.c
  drv_iface.c
  api.c
  )
zephyr_sources_ifdef(CONFIG_WIFIMGR_CLI cli.c)
zephyr_sources_ifdef(CONFIG_WIFIMGR_CONFIG config.c)
zephyr_sources_ifdef(CONFIG_WIFIMGR_DHCPC dhcpc.c)
zephyr_sources_ifdef(CONFIG_WIFIMGR_LED led.c)
