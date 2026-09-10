# Testing

The ESP32 WT32-ETH01 MAVLink Bridge was previously tested on real hardware using the working firmware version on which this repository is based.

## Hardware

- WT32-ETH01
- ESP32
- LAN8720 Ethernet PHY
- ArduPilot flight controller
- Ethernet network
- QGroundControl

## Ethernet

The following functionality was verified on real hardware:

- LAN8720 Ethernet connection
- Static IP configuration
- Device accessible over Ethernet
- Web configuration interface accessible from a PC

## UART

Tested UART connection:

- GPIO14 TX → Flight Controller RX
- GPIO35 RX ← Flight Controller TX
- Baudrate: `115200`

## MAVLink

MAVLink telemetry from the flight controller was successfully transferred through the WT32-ETH01 Ethernet interface and received by QGroundControl.

Data path:

`Flight Controller → UART → ESP32 → LAN8720 → Ethernet / UDP → QGroundControl`

Default UDP port:

`14550`

## Web Interface

The built-in web interface was used for network and UART configuration.

Default address:

`http://192.168.88.50`

Available settings include:

- Device IP
- Target IP
- Subnet Mask
- UDP Port
- UART Baudrate

Settings are stored in ESP32 non-volatile storage.

## Current Repository Firmware

The firmware published in this repository is based on the previously tested working version.

The public version has been cleaned and rebranded from the original development firmware to `bmax_sys`.

The cleaned repository version has not yet been re-tested on hardware after the branding cleanup.

## Result

The original working implementation successfully demonstrated:

- UART communication with an ArduPilot flight controller
- Ethernet communication through LAN8720
- MAVLink telemetry over UDP
- QGroundControl telemetry reception
- Built-in web configuration interface

---

**bmax_sys**

Embedded • Networking • Robotics
