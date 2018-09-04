zephyr_compile_options(
  )

zephyr_sources_ifdef(
  CONFIG_WIFIMGR
  wifimgr.c
  state_machine.c
  cmd_prcs.c
  evt_lsnr.c
  sta.c
  ap.c
  drv_iface.c
  api.c
  )
zephyr_sources_ifdef(CONFIG_WIFIMGR_CLI cli.c)
