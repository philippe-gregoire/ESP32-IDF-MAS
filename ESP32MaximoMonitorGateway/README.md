ESP32-IDF IBM Maximo Application Suite example gateway client
====================

This example is to be used with [Espressif IoT Development Framework](https://github.com/espressif/esp-idf).

# Usage
This project uses a SPIFFS partition to store configuration files.

The files available in the `spiffs` folder will be flashed to the SPIFFS partition on the ESP32.

> **NOTE**
This file system holds a folder named `secret` where configuration files are maintained, containing among others userids and passwords. The contents of `secret` is purposedly *not* comittted to git, and its contents should be created for each project.

The expected contents of `secret` folder for this project is a set of one-liner files:
### Definitions for local connectivity:
* `wifi_ssid.txt`: SSID for Wifi
* `wifi_pass.txt`: Password for Wifi
### Definition from WIoTP/Maximo Monitor:
* `wiotp_orgid.txt`: Maximo Monitor/WIoTP orgID
* `wiotp_gw_type.txt`: Type of the gateway as defined in WIoTP
* `wiotp_gw_id.txt`: ID of the gateway instance as defined in WIoTP
* `wiotp_gw_token.txt`: Type of the gateway as defined in WIoTP
* `wiotp_dev_id.txt`:: Type of the device as defined in WIoTP
* `wiotp_dev_type.txt`: Type of the device as defined in WIoTP
