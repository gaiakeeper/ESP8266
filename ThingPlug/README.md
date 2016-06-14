# ThingPlug
This is an example ThingPlug device on ESP8266. This is derived from [SKT-ThingPlug starter kit](https://github.com/SKT-ThingPlug/thingplug-starter-kit).

# Required Libraries
ESP8266 nonOS SDK doesn't support standard libraries such as libc. Therefore, it's hard to use other open source libraries, generally working on Linux.

ThingPlug requires XML parser. I modified [YXML parser](https://dev.yorhel.nl/yxml) and put it into [Libraries](../Libraries). I created XML wrapper for ThingPlug test. But, it's not completed yet.

# Required Configrations
This supports "Smart Config". Therefore, you may use "ESP8266 Smart Config" tool and set up SSID/PASSWORD for your Wi-Fi AP. The Wi-Fi information is stored in FLASH. Next time, it will be used. If you want to remove it, you have only to flashinit in Makefile.

And next, you should create your own device ID. Phone number is recommended for the unique.
```
const char* deviceID = "0.2.481.1.101.01099889911";
```

# Required FLASH
Your ESP8266 module should have more than 1M FLASH. You can specify FLASH size and modify LD script in Makefile.

```
SPI_SIZE_MAP ?= 2
LD_SCRIPT = eagle.app.v6.1024.ld
```

You can create "eagle.app.v6.1024.ld" from "eagle.app.v6.ld" in ESP8266_SDK/ld folder and modify it as below.

```
MEMORY
{
  dport0_0_seg :                        org = 0x3FF00000, len = 0x10
  dram0_0_seg :                         org = 0x3FFE8000, len = 0x14000
  iram1_0_seg :                         org = 0x40100000, len = 0x8000
  irom0_0_seg :                         org = 0x40240000, len = 0xBC000
}
```

It increase the size of program area (eagle.irom0text.bin) from 0x4C000 to 0xBC000.

# Build

You can import the project from eclipse and build all by Makefile target. Before flashing, you should specify COM port to connect your ESP8266 in Makefile.

```
ESPPORT ?= COM6
```