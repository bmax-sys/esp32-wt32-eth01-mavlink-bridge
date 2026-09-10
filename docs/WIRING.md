# Wiring

Tested hardware configuration for the **bmax_sys ESP32 WT32-ETH01 MAVLink Bridge**.

## WT32-ETH01 ↔ Flight Controller

| WT32-ETH01 | Flight Controller | Function |
|---|---|---|
| GPIO14 (TX) | RX | UART TX |
| GPIO35 (RX) | TX | UART RX |
| 5V | 5V | Power |
| GND | GND | Common Ground |

## UART Connection

UART lines must be crossed:

`GPIO14 TX → Flight Controller RX`

`GPIO35 RX ← Flight Controller TX`

Default UART baudrate:

`115200`

## Ethernet

The WT32-ETH01 uses an integrated **LAN8720 Ethernet PHY**.

The Ethernet connection is provided through the RJ45 interface on the board.

## Data Path

`Flight Controller ↔ UART ↔ ESP32 ↔ LAN8720 ↔ Ethernet / UDP ↔ QGroundControl`

## Default Network Configuration

| Parameter | Default |
|---|---|
| Device IP | `192.168.88.50` |
| Target IP | `192.168.88.255` |
| Subnet Mask | `255.255.255.0` |
| UDP Port | `14550` |
| UART Baudrate | `115200` |

## Web Interface

Default address:

`http://192.168.88.50`

Authentication:

- Username: `spx`
- Password: `1488`

The web interface allows configuration of:

- Device IP
- Target IP
- Subnet Mask
- UDP Port
- UART Baudrate

Settings are stored in ESP32 non-volatile storage.

After saving the configuration, the ESP32 automatically restarts.

## Important

A common ground between the WT32-ETH01 and flight controller is required.

UART TX and RX lines must be crossed.

Verify the power requirements and pinout of your specific WT32-ETH01 board before connecting power.

---

**bmax_sys**

Embedded • Networking • Robotics
