**English** | [Русский](README.ru.md)

![4VRS Display — People, Technology, Better Together](assets/github-banner.png)

# 4VRS Display

**Small display. Big possibilities.**

4VRS Display is an open-source ESP32 display for Home Assistant. It brings room temperatures, humidity, lighting, ventilation, water-leak alerts and other entity states to a compact screen wherever you need them: a living room, garage or server room.

Choose the entities in Home Assistant; the integration sends their states through MQTT, and the display groups them by room. Configure the device in your browser and update its firmware over Wi-Fi without removing it from its installation.

**Stable firmware: 0.4.4 · Home Assistant integration: 0.4.0 · License: MIT**

[Download the release](https://github.com/dk-1983/4VRS-Display/releases/tag/firmware-v0.4.4) · [Report an issue](https://github.com/dk-1983/4VRS-Display/issues)

## Features

- **Up to 20 entities per display**, selected and edited in Home Assistant.
- **Room-by-room presentation**: a full-screen room title and icon, followed by pages of up to three cards. Different rooms never share a page.
- **Meaningful icons and states** for temperature, humidity, lights, fans, valves, leak sensors and other equipment, with a text fallback for unrecognized types.
- **Data freshness indicators** that distinguish unavailable, unknown and stale data from an inactive device.
- **Local web configuration** for Wi-Fi, MQTT, language, credentials and updates, with an About page for device information.
- **English by default and Russian as an option** in the web interface.
- **A built-in static screensaver** stored in firmware, usable without a memory card.
- **LAN ArduinoOTA and signed GitHub updates**, with two application slots, boot validation and rollback support.
- **Per-device automatic-update controls** in the web interface and Home Assistant integration.

The current release displays entity states; operating physical equipment from the display is not implemented.

## How it works

```text
Home Assistant → MQTT broker → 4VRS Display
Browser        → local web interface → device settings
GitHub release → signed update feed → firmware update
```

Each display has its own identity, pairing key and entity selection. Room membership comes from the entity's area, then its device's area; entities without an area form a separate group.

For example, a room with five entities shows its cover, then pages of **3 + 2** cards. A second room with four entities shows its own cover, then **3 + 1**. The sequence repeats automatically. Room covers last three seconds and card pages eight seconds.

The integration sends snapshots when states change and every 30 seconds. After 90 seconds without a fresh snapshot, the display marks the data as stale.

## Supported hardware

The current firmware targets **ESP32-WROVER with 4 MiB PSRAM**, the **NADIM V5** baseboard and a **240 × 320 SPI ILI9341** display in portrait orientation. This is a specific hardware profile; other ESP32 boards and displays may require changes.

| Display signal | ESP32 GPIO |
| --- | --- |
| CS | 13 |
| DC / A0 | 14 |
| RESET | 2 |
| MOSI / SDA | 23 |
| SCK | 18 |
| SDO / MISO, optional diagnostics | 19 |
| LED logic input (module has a transistor) | 4 |

A touchscreen and microSD card are not required. GPIO signals use 3.3 V logic; follow the display module's power specifications and connect GPIO4 to the module's LED control input, which drives its onboard transistor.

## Manufacturer documentation

- [LCDWIKI: 2.4-inch SPI ILI9341 module](https://www.lcdwiki.com/2.4inch_SPI_Module_ILI9341_SKU:MSP2402) — module schematic, user manual and mechanical drawings.
- [Espressif documentation catalogue](https://www.espressif.com/en/support/documents/technical-documents) — select the datasheet matching the exact module marking.
- [ESP32-WROVER-E / WROVER-IE datasheet](https://documentation.espressif.com/esp32-wrover-e_esp32-wrover-ie_datasheet_en.html) and [ESP32 hardware design guidelines](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32/schematic-checklist.html).
- [ESP LINK v1.0 documentation from IOT-MCU](https://github.com/IOT-MCU/ESP-LINK-v1.0) — an example USB-UART adapter, not a required model. Its ESP-01 instructions are not the ESP32 flashing procedure.
- [Base circuit and optional extended schematic (A1), BOM and assembly notes](hardware/schematic/README.md).

## First setup

1. Download `4vrs-display-0.4.4-factory.bin` from the release. For the initial UART installation, select **ESP32** in the Espressif flashing tool and write the factory image at **0x0**. This replaces data in the image's flash region; use OTA for an already configured device.
2. Enter the board's bootloader mode, flash the image, wait for verification, then reset normally without holding BOOT/PGM.
3. Connect to the setup Wi-Fi network `4vrs-rack-<id>-setup`. Its password is **`KIaE18TTn4Omp8H-0peXAk1i`**.
4. Open **http://192.168.4.1/**, select your **2.4 GHz Wi-Fi** network and enter its password.
5. Find the device's assigned IP in your router's DHCP client list and open it in a browser. A DHCP reservation keeps its address consistent. The setup access point turns off after a stable LAN connection.
6. Sign in with **username `admin`, password `admin`**. Change these in **Settings**, where you can also select the interface language. Web, Wi-Fi and ArduinoOTA credentials are separate.
7. Open **MQTT** and enter your broker's host without `http://`, port, username and password. Use the same broker as Home Assistant.

When MQTT is disabled, the built-in screensaver is displayed.

## Home Assistant integration

1. Configure Home Assistant's MQTT integration.
2. Download `fourvrs_display-0.4.0.zip` from the release and copy its `custom_components/fourvrs_display` directory into your HA configuration's `custom_components` directory.
3. Restart Home Assistant and add the **4VRS Display** integration.
4. Enter the **Device ID** and **pairing key** shown on this display's authenticated MQTT configuration page.
5. Select **1–20 different entities** and save. Their states and room information will be sent to the display.

To edit the selection later, open **Settings → Devices & services → Integrations → 4VRS Display** and use the integration entry's **gear icon**. You do not need to delete and recreate the connection. The pencil on the device page edits device metadata, not the displayed entity list.

`connected: true` confirms a broker connection. If `has_snapshot: false` remains, check pairing, the selected entities and that HA and the display use the same broker.

## Firmware updates

Use the **application image** `4vrs-display-0.4.4.bin` for OTA; the factory image is for initial UART installation. ArduinoOTA remains available on the local network, with an individual password configurable in **Settings → ArduinoOTA**.

GitHub updates use a signed stable manifest. Before installation, the firmware checks the signature, hardware profile, version, image size and SHA-256 digest. It writes to the inactive application slot and validates the next boot, with rollback support for failed startup.

Automatic updates are enabled by default and can be blocked for an individual display through the web interface or Home Assistant. Either block prevents automatic installation. Once HA manages the update policy, fresh permission from HA is required. A new source commit alone does not trigger installation: an update must be published through the signed stable feed.

## Planned capabilities

The next stages include GIF playback and 4VRS animations, user media on microSD, media upload through the web interface, a standalone photo-frame mode, local sensors and navigation using three physical buttons. These are planned features, not functions of the current release. The embedded image will remain the fallback when user media is unavailable.

## Development and documentation

| Directory | Purpose |
| --- | --- |
| `firmware/rack_bootstrap/` | ESP32 firmware and embedded web interface |
| `home_assistant/custom_components/fourvrs_display/` | Home Assistant integration |
| `protocol/` | Shared MQTT contract, JSON schemas and examples |
| `tools/` | Build, OTA and release tools |
| `assets/` | Images and project artwork |
| `docs/` | User and development guides |

The build uses Arduino CLI, Arduino-ESP32 **3.3.8**, Adafruit GFX and Adafruit ILI9341 with their dependencies. Supply your Arduino CLI configuration file:

```sh
python tools/build.py --config <arduino-cli.yaml> --version 0.4.4
```

Run the integration and release checks:

```sh
python -m unittest discover -s home_assistant/tests -v
python -m pip install -r tools/requirements-release.txt
python tools/test_release.py
```

Detailed guides are currently in Russian: [first setup](docs/user-guide.md), [hardware](docs/hardware.md), [Home Assistant](docs/home-assistant.md), [updates](docs/updates.md), [development](docs/development.md) and [roadmap](docs/plan.md). The full Russian version of this overview is available through the language link at the top.

Bug reports and contributions are welcome. Include component versions, hardware details and steps to reproduce; remove passwords and pairing keys from logs before sharing them.

## License

Original project code and documentation are distributed under the [MIT License](LICENSE). Third-party libraries retain their own licenses.

### Display and network preferences

- **Settings → Show room covers** turns the room title/icon screens on or off. It defaults to on and survives reboot and OTA. Turning it off keeps the room name in each page header and never mixes rooms.
- **About → Check display** reads controller registers and explains the result. Connect SDO/MISO to GPIO19 for readback; this checks communication, not the LCD glass or backlight.
- **IPv4 settings** selects DHCP (default) or a static IP, subnet mask, gateway and two DNS servers. Gateway/DNS may be empty for an isolated LAN; GitHub updates require internet access and DNS. Use an unused address outside the dynamic pool or reserve it in your router. The setup AP subnet 192.168.4.0/24 is reserved.
- Address changes are tried for three minutes. Open `/network` at the new LAN address and click **Confirm connection** to save. Without confirmation, or after a power cycle before confirmation, the previous settings return. Setup Wi-Fi remains available during the trial; firmware updates are paused until it ends. Configure Wi-Fi first when setting up a new board.
- During firmware installation the TFT shows progress, followed by verification/restart. The first upgrade from an older firmware gains this screen only for subsequent updates.
