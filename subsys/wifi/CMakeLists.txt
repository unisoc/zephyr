zephyr_compile_options(
  )

zephyr_sources_ifdef(
  CONFIG_WIFIMGR
  wifimgr.c
  ap.c
  api.c
  cmd_prcs.c
  drv_iface.c
  evt_lsnr.c
  sm.c
  sta.c
  )
zephyr_sources_ifdef(CONFIG_WIFIMGR_CLI cli.c)
zephyr_sources_ifdef(CONFIG_WIFIMGR_CONFIG_SAVING config.c)
zephyr_sources_ifdef(CONFIG_WIFIMGR_DHCPC dhcpc.c)
zephyr_sources_ifdef(CONFIG_WIFIMGR_LED led.c)
