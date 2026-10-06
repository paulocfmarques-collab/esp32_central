# ESP32 Central

<div align="center">

![Banner](https://dummyimage.com/1400x420/0f172a/e2e8f0&text=ESP32+Central+%7C+Touchscreen+Automation+Hub)

**ESP32 touchscreen control hub for local and remote device orchestration**

[![Platform](https://img.shields.io/badge/platform-ESP32-FF6F00?style=for-the-badge&logo=arduino)](https://github.com/paulocfmarques-collab/esp32_central)
[![Language](https://img.shields.io/badge/language-C%2B%2B-00599C?style=for-the-badge&logo=c%2B%2B)](https://github.com/paulocfmarques-collab/esp32_central)
[![Firmware](https://img.shields.io/badge/firmware-Arduino%20IDE-00979D?style=for-the-badge&logo=arduino)](https://github.com/paulocfmarques-collab/esp32_central)
[![Connectivity](https://img.shields.io/badge/connectivity-Wi--Fi%20%2B%20UDP-00A3FF?style=for-the-badge)](https://github.com/paulocfmarques-collab/esp32_central)
[![Display](https://img.shields.io/badge/display-TFT%20Touch-9C27B0?style=for-the-badge)](https://github.com/paulocfmarques-collab/esp32_central)
[![Status](https://img.shields.io/badge/status-portfolio%20ready-success?style=for-the-badge)](https://github.com/paulocfmarques-collab/esp32_central)
[![Repo Size](https://img.shields.io/github/repo-size/paulocfmarques-collab/esp32_central?style=for-the-badge)](https://github.com/paulocfmarques-collab/esp32_central)
[![Last Commit](https://img.shields.io/github/last-commit/paulocfmarques-collab/esp32_central?style=for-the-badge)](https://github.com/paulocfmarques-collab/esp32_central/commits/main)

</div>

> A polished embedded control experience for ESP32: touchscreen-driven, network-aware, and designed for real-world automation dashboards.

---

> [!NOTE]
> This project is presented in a product-style format to highlight architecture, capability, and polish for portfolio review.

> [!TIP]
> Add real screenshots in `docs/screenshots/` and a short demo GIF in `docs/demo/` to make the page feel production-ready.

> [!IMPORTANT]
> This solution is intended for trusted local networks; UDP traffic is not encrypted and the web portal ships without authentication by default.

---

## Quick Highlights

- Touch-first interface for navigation, diagnostics, and command execution
- **LOCAL** mode for on-device control and troubleshooting
- **REMOTE** mode for UDP-based orchestration of slave devices
- Support for up to **4 slave nodes** on the LAN
- Wi-Fi configuration portal at `192.168.4.1`
- NTP time sync, persistence, and OTA support
- Device health monitoring: CPU, RAM, flash, temperature, network, uptime

---

## Screenshots & Demo

| Preview | What it shows |
| --- | --- |
| `docs/screenshots/home.png` | Main dashboard / home screen |
| `docs/screenshots/remote-mode.png` | Remote command selection and execution |
| `docs/screenshots/config-ap.png` | Access point configuration portal |
| `docs/demo/demo.gif` | Short interaction demo |

**Demo video:** `https://your-demo-link-here`

---

## What Makes It Stand Out

<div align="center">

| Embedded UI | Network Control | Product-Ready Docs |
| --- | --- | --- |
| Touchscreen-driven UX | UDP command routing | Portfolio-friendly presentation |

</div>

> [!TIP]
> A strong README should answer three questions immediately: what it does, how it works, and why it is impressive.

> [!NOTE]
> The sections below are organized like a product landing page: value proposition first, then proof, then setup.

---

## Repository Map

| File | Purpose |
| --- | --- |
| `Central.ino` | Firmware entry point, initialization, and main loop |
| `Botao.h` | UI button structure and representation |
| `CommandHandler.cpp` / `CommandHandler.h` | Local and remote command processing |
| `Display.cpp` / `Display.h` | TFT display rendering helpers |
| `ScreenRenderer.cpp` / `ScreenRenderer.h` | Screen composition and visual rendering |
| `TouchDriver.cpp` / `TouchDriver.h` | Touch input reading and handling |
| `InterfaceCentral.cpp` / `InterfaceCentral.h` | Main control interface organization |
| `LayoutDatabase.cpp` / `LayoutDatabase.h` | UI layout and page catalog |
| `UdpComm.cpp` / `UdpComm.h` | UDP communication with slave devices |
| `WifiConfig.cpp` / `WifiConfig.h` | Wi-Fi configuration and persistence |
| `NTPUtil.h` | NTP time synchronization helpers |
| `README.md` | Project documentation |
| `CHANGELOG.md` | Professional change history |

---

## Architecture

```mermaid
graph LR
    U[User] --> T[Touch TFT Display]
    T --> C[ESP32 Central]
    C --> W[Wi-Fi LAN]

    W --> S1[Slave 1]
    W --> S2[Slave 2]
    W --> S3[Slave 3]
    W --> S4[Slave 4]

    S1 --> R[UDP Response]
    S2 --> R
    S3 --> R
    S4 --> R

    R --> C
    C --> T
```

---

## Hardware Recommended

| Component | Specification |
| --- | --- |
| Processor | ESP32 with Wi-Fi |
| Display | 320×240 TFT SPI |
| Touch | XPT2046 |
| Status LED | GPIO 4 |
| Power | Stable 5V supply |
| Network | Local 2.4 GHz Wi-Fi |

---

## Installation

1. Clone the repository:

```bash
git clone https://github.com/paulocfmarques-collab/esp32_central.git
cd esp32_central
```

2. Install the required Arduino IDE libraries:

- `TFT_eSPI`
- `XPT2046_Touchscreen`

3. Configure the display inside the `TFT_eSPI` library setup files.

4. Select **ESP32 Dev Module** and upload the firmware.

---

## First Access

1. The ESP32 creates the access point `ESP32_CENTRAL_CONFIG`
2. Connect to the AP
3. Open `192.168.4.1`
4. Enter the SSID, password, and optionally the slave IPs
5. Save the configuration and wait for the reboot

---

## Commands

### Local commands

- `help`
- `info`
- `status`
- `reason`
- `version`
- `build`
- `cpu`
- `ram`
- `flash`
- `temp`
- `psram`
- `mac`
- `net_info`
- `time`
- `date`
- `led_on`
- `led_off`
- `led_blink:500`
- `set_escravo1:IP`
- `set_escravo2:IP`
- `set_escravo3:IP`
- `set_escravo4:IP`
- `set_fuso:-3`
- `scan`
- `reset_wifi`
- `desligar`
- `lastcmd`
- `cmdcount`
- `vago`

### Remote commands via UDP

- `LED_ON`
- `LED_OFF`
- `LED_BLINK:1000`
- `TEMP`
- `CPU`
- `RAM`
- `FLASH`
- `INFO`
- `STATUS`
- `VERSION`
- `UPTIME`
- `NET_INFO`
- `TIME`
- `DATE`
- `alive`

---

## Advanced Capabilities

### NTP
Automatic time synchronization with configurable timezone.

### OTA
Remote firmware update support over the network.

### Persistence
Network and state settings are stored for recovery after restart.

### Asynchronous LED
The status LED can blink without blocking the main loop.

---

## Troubleshooting

| Problem | Solution |
| --- | --- |
| Black screen | Check TFT wiring and pin mapping |
| Touch not working | Verify XPT2046 wiring and calibration |
| Wi-Fi won’t connect | Open `192.168.4.1` and validate SSID/password |
| No UDP response | Confirm all slaves are on the same network |
| NTP not syncing | Check internet access and DNS |
| OTA fails | Reboot the ESP32 and confirm available flash space |

---

## Security

> [!WARNING]
> This project is intended for trusted local networks.

- UDP is not encrypted
- The web portal has no default authentication
- Do not expose the device directly to the internet
- Do not store real credentials in public repositories

---

## Changelog

See [`CHANGELOG.md`](./CHANGELOG.md) for the full professional history of changes.

---

## Author

**Paulo Marques**  
GitHub: [@paulocfmarques-collab](https://github.com/paulocfmarques-collab)

---

## Contributing

Found a bug or have an idea? Open an issue or submit a pull request.

---

<div align="center">

**Built with ❤️ for the ESP32 community**

![Stars](https://img.shields.io/github/stars/paulocfmarques-collab/esp32_central?style=social)
![Forks](https://img.shields.io/github/forks/paulocfmarques-collab/esp32_central?style=social)
![Issues](https://img.shields.io/github/issues/paulocfmarques-collab/esp32_central?style=social)

</div>
