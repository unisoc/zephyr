
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(app PRIVATE src/main.c)

target_sources_ifdef(
  CONFIG_BT
  app PRIVATE
  src/bluetooth/blues.c
  src/bluetooth/throughput.c
  src/bluetooth/wifi_manager_service.c
  src/bluetooth/mesh.c
  )