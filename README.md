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

> [!NOTE]
> Demo link and real screenshots are pending publication in this repository.

---

## What Makes It Stand Out

<div align="center">

| Embedded UI | Network Control | Product-Ready Docs |
| --- | --- | --- |
| Touchscreen-driven UX | UDP command routing | Portfolio-friendly presentation |

</div>

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

| Command | Description |
| --- | --- |
| `help` | Shows available local commands |
| `info` | Displays firmware and board summary |
| `status` | Displays system status |
| `reason` | Shows last reset reason |
| `version` | Shows firmware version |
| `build` | Shows build metadata |
| `cpu` | Shows CPU information |
| `ram` | Shows memory usage |
| `flash` | Shows flash usage |
| `temp` | Shows chip temperature |
| `psram` | Shows PSRAM info |
| `mac` | Shows MAC address |
| `net_info` | Shows network information |
| `time` | Shows current time |
| `date` | Shows current date |
| `led_on` | Turns on status LED |
| `led_off` | Turns off status LED |
| `led_blink:500` | Blinks status LED with interval |
| `set_escravo1:IP` | Sets slave 1 IP |
| `set_escravo2:IP` | Sets slave 2 IP |
| `set_escravo3:IP` | Sets slave 3 IP |
| `set_escravo4:IP` | Sets slave 4 IP |
| `set_fuso:-3` | Sets timezone offset |
| `scan` | Scans Wi-Fi networks |
| `reset_wifi` | Clears Wi-Fi config |
| `desligar` | Triggers shutdown routine |
| `lastcmd` | Shows last command |
| `cmdcount` | Shows command counter |
| `vago` | Reserved/diagnostic command |

### Remote commands via UDP

| Command | Description |
| --- | --- |
| `LED_ON` | Turns on remote LED |
| `LED_OFF` | Turns off remote LED |
| `LED_BLINK:1000` | Blinks remote LED |
| `TEMP` | Returns remote temperature |
| `CPU` | Returns remote CPU info |
| `RAM` | Returns remote RAM info |
| `FLASH` | Returns remote flash info |
| `INFO` | Returns remote firmware summary |
| `STATUS` | Returns remote system status |
| `VERSION` | Returns remote firmware version |
| `UPTIME` | Returns remote uptime |
| `NET_INFO` | Returns remote network info |
| `TIME` | Returns remote current time |
| `DATE` | Returns remote current date |
| `alive` | Returns liveness/heartbeat response |

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

## Versioning and Releases

- Current workflow uses `main` as the integration branch.
- Recommended release pattern: `vMAJOR.MINOR.PATCH` (example: `v1.0.1`).
- Create annotated tags for each production-ready firmware baseline.
- Register each release with summary notes and compatibility details.

Quick commands:

```bash
git tag -a v1.0.1 -m "Release v1.0.1"
git push origin v1.0.1
```

---

## License

No `LICENSE` file is currently present in this repository.
If you want to publish as open-source, add a license file (for example, MIT) and update this section accordingly.

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
