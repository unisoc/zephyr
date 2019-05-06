.. _wifi_sta_rtt:

Wi-Fi: RTT range
####################

Overview
********

Application demonstrating usage of STA RTT range of Wi-Fi manager.
To perform an RTT, the necessary steps are as follows.
1. Get STA config
2. Open STA
3. Scan the APs which support RTT
4. Assemble RTT range request by scan results
5. Send the RTT range request
6. Receive the RTT range response

Requirements
************

* An access point with IEEE 802.11 a/b/g/n/ac support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/wifi/rtt` in
the Zephyr tree.

See :ref:`wifi samples section <wifi-samples>` for details.
