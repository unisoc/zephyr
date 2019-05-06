.. _wifi_repeater:

Wi-Fi: Repeater
####################

Overview
********

Application demonstrating usage of STA&AP APIs of Wi-Fi manager.
To start WiFi repeater, the necessary steps are as follows.
1. Get STA config to obtain the specified AP
2. Open STA
3. Scan the specified AP
4. Connect to the specified AP
5. Get AP config
6. Open AP
7. Start AP
Note that the packet forwarding is accomplished by Wi-Fi firmware.

Requirements
************

* An access point with IEEE 802.11 a/b/g/n/ac support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/wifi/repeater` in
the Zephyr tree.

See :ref:`wifi samples section <wifi-samples>` for details.
