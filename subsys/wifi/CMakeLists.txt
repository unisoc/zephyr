zephyr_compile_options(
  )

zephyr_sources_ifdef(
  CONFIG_WIFIMGR
  ap.c
  api.c
  autorun.c
  cmd_prcs.c
  drv_iface.c
  evt_lsnr.c
  sm.c
  sta.c
  wifimgr.c
  )
zephyr_sources_ifdef(CONFIG_WIFIMGR_CLI cli.c)
zephyr_sources_ifdef(CONFIG_WIFIMGR_CONFIG_SAVING config.c)
zephyr_sources_ifdef(CONFIG_WIFIMGR_PSK psk.c)
zephyr_sources_ifdef(CONFIG_WIFIMGR_DHCPC dhcpc.c)
zephyr_sources_ifdef(CONFIG_WIFIMGR_LED led.c)

if(CONFIG_WIFIMGR_PSK)
  zephyr_sources(psk.c)
else()
  assert(CONFIG_MBEDTLS_LIBRARY "WIFIMGR_PSK was disabled!")
endif()
