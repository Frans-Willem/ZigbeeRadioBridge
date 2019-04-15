# Zigbee Packet Bridge

This is an open-source bridge between the Zigbee/802.15.4 radio interface on a Texas Instruments CC2531 USB dongle and a host computer. While other solutions to communicate with Zigbee networks using this dongle exist (like TI's Zigbee Network Processor), they often attempt to do all Zigbee handling on the dongle's internal 8051 chip.

Zigbee Packet Radio aims to offload as much as possible onto the computer, and have the CC2531 act as only a dumb radio slave.

## Getting started

At this point, the rough implementation appears to work, but there is nothing actually using this. However, feel free to use this in Zigbee related projects.

### Libraries and tools used

Zigbee Radio Bridge uses [Contiki OS](http://www.contiki-os.org/) for it's Radio driver and infrastructure. Furthermore it uses the [Small Device C Compiler (SDCC)](http://sdcc.sourceforge.net/) as the compiler, but is quite picky about which version it will work on. The 3rdparty directory contains scripts to get and set up working versions of Contiki and SDCC, as well as set up aliases such that Python 2 is always used (Contiki is picky about this).

### Compiling
To compile, execute the following commands:
```
cd "ZigbeeRadioBridge"
./3rdparty/compileSdcc.sh
./3rdparty/getContiki.sh
source ./3rdparty/setupEnvironment.sh
make
```
Afterwards you should have a file named "radio\_bridge.hex" that you can flash to the dongle using [cc-tool](https://github.com/dashesy/cc-tool/).

## License
Zigbee Radio Bridge is licensed under the GNU General Public License, version v3.0. See LICENSE-gpl-3.0.txt or the [online version](https://www.gnu.org/licenses/gpl-3.0.txt) for more information.

Contiki is licensed under the [3-clause BSD license](https://github.com/contiki-os/contiki/blob/master/LICENSE).
