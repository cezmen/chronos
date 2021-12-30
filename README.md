

Chronos is an utility software for testing FTM (Fine Time Measurement) on Espressif ESP32-S2 devices

## How to use the Chronos Utility

### [1] Set Target

```
idf.py set-target esp32s2
```


### [2] Configure the Project

```
idf.py menuconfig
```


### [2.1] Set Configuration Parameters in the folowing menus :

- Example Configuration->Soft AP
- Example Configuration->TCP Server
- Example Configuration->FTM
  
| Parameter | Description | Example | Module |
| ----------- | ----------- | ----------- | -----------|
| ESP_WIFI_SSID | WiFi SSID| myssid | SoftAP |
| ESP_WIFI_PASSWORD | WiFi Password | mypassword | SoftAP |
| ESP_WIFI_CHANNEL | WiFi Channel | 7 | SoftAP |
| ESP_MAX_STA_CONN | Maximal STA connections | 4 | SoftAP |
| ESP_INTERFACE_IP | IPv4 Address | 192.168.4.1 | SoftAP |
| ESP_INTERFACE_GW | Gateway IPv4 Address | 192.168.4.1 | SoftAP |
| ESP_INTERFACE_NETMASK | Netmask | 255.255.255.0 | SoftAP |
| ESP_IPV4 | IPV4 (y/n)  | y | TCP Server |
| ESP_IPV6 | IPV6 (y/n)  | n | TCP Server |
| ESP_PORT | Port | 5000 | TCP Server |
| ESP_KEEPALIVE_IDLE | TCP keep-alive idle time(s) | 5 | TCP Server |
| ESP_KEEPALIVE_INTERVAL | TCP keep-alive interval time(s) | 5 | TCP Server |
| ESP_KEEPALIVE_COUNT | TCP keep-alive packet retry send counts | 5 | TCP Server |
| ESP_FTM_REPORT_LOG_ENABLE | FTM Report logging (y/n)| y |  FTM |
| ESP_FTM_REPORT_SHOW_DIAG | Show dialog tokens (y/n)| y | FTM |
| ESP_FTM_REPORT_SHOW_RTT| Show RTT values (y/n)| y | FTM |
| ESP_FTM_REPORT_SHOW_T1T2T3T4 | Show T1 to T4 (y/n)| y | FTM |
| ESP_FTM_REPORT_SHOW_RSSI | Show RSSI levels (y/n)| y | FTM |


### [2.2] Aditional Parameters Setup

Component Config -> WiFi ->
  - WiFi FTM : **y**
  - FTM Initiator Support : **y**
  - FTM Responder Support : **y**

Serial Flasher Config -> 
  - Flash Size : **4MB**
  - After Flashing : **Stay In Bootloader**

Component Config -> Common ESP Related ->
  - Channel for Console Output : **USB CDC** ( if using Franzinho WiFi) or **UART0** ( if using ESP32-S2-Devkit-C )
  

### [3] Build

Build the project :

```
idf.py build
```

### [4] Flash

Put the board in DFU mode ( by pressing BOOT and RESET keys in the following sequence : press BOOT, press RESET, release RESET, release BOOT ).

Flash the firmware to the board :

```
idf.py -p <device name> flash
```
(Note : Use 'ls /dev/tty*' to discover the exact **device name** in your environent)


### [5] Monitor ( optional )

Reset the board ( by pressing the RESET key ).    


### [5.1] Monitor the Franzininho WiFi board (through USD CDC)

This Demo doesn't work well with "idf.py monitor" when the Console Output is using **USB CDC** port.

In this case, use a serial terminal emulator (such as screen) instead :

```
screen <device name> 115200,cs8 
```
(Note : Use 'ls /dev/tty*' to discover the exact **device name**)

(To exit **screen**, type ``Ctrl-A with k``, pressing ``y`` right after to kill the window).


### [5.2] Monitor the ESP32-S2-Devkit-C board (through UART0)

Run the ESP-IDF monitor :

```
idf.py -p <device name>  monitor
```
(Note : Use 'ls /dev/tty*' to discover the exact **device name** in your environent)

(To exit the serial monitor, type ``Ctrl-]``.)

### [6] Test Remotely (via TCP/IP)

### [6.1] Using a smartphone or PC, establish a WiFi connection to the ESP32-S2 Access Point (SSID="myssid", PASSWORD="mypassword")


### [6.2] Using a TCP/IP terminal emulator App ( or **telnet** command in the PC ), establish a TCP/IP connection to the ESP32-S2 ( IP=192.168.4.1, port=5000 )


### [6.3] Once connected (via TCP/IP), send commands ( getting their respective responses )

| Command | Description | Example | 
| ----------- | ----------- | ----------- |
| WiFi Scan | scan nearby WiFi stations | { "function" : "scan" } ; |
| WiFi Scan by SSID | scan specific WiFi station | { "function" : "scan" , "parameters" : { "ssid" : "FTM-ST-1" }} ; |
| FTM by SSID | FTM procedure | { "function" : "ftm" , <br />"parameters" : { "ssid" : "FTM-ST-1" }} ; |
| FTM by MAC  | FTM procedure | { "function" : "ftm" , <br />"parameters" : { "mac" : "7c:df:a1:40:ce:55" , "channel" : 13 }} ; |
| Custom FTM | FTM procedure with <br /> custom parameters | { "function" : "ftm" , <br />"parameters" : { "ssid" : "FTM-ST-1" , "count" : 8, "burst" : 16}} ; |

