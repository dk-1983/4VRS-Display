**English** | [Русский](README.ru.md)

# 4VRS Display electrical schematic - Rev A1

## Base and extended variants

**Base variant:** the author's original circuit, used in the assembled, working devices. It retains the original L1117-33 supply and component population, with our ILI9341 connections and direct GPIO4-to-LED control input.

**Extended variant (A1):** the PDF, SVG, BOM and connectivity list below describe an optional alternative with AP7361C and additional supply/startup components. The author reviewed this schematic and accepted its circuit design. It is not the component list of the existing boards or a required upgrade.

The base circuit remains the project's reference implementation. The added complexity of A1 is optional; no comparative stability measurements have established its benefit for this installation. Working boards do not need to be rebuilt to match A1. Three buttons remain provisions for future firmware.

[Download the three-sheet A3 PDF](4vrs-display-schematic-A1.pdf)

![Power, ESP32 and ILI9341](4vrs-display-schematic-A1-sheet-1.svg)

## Contents

- [Sheet 1: power, controller and display](4vrs-display-schematic-A1-sheet-1.svg)
- [Sheet 2: reset, UART and backlight](4vrs-display-schematic-A1-sheet-2.svg)
- [Sheet 3: optional buttons, BOM and assembly notes](4vrs-display-schematic-A1-sheet-3.svg)
- [Component list](bom.csv), [connectivity list](connections.json), [drawing source](generate_schematic.py).

This is a circuit design based on the supplied NADIM V5 fragments and LCDWIKI module documentation. The confirmed display wiring is retained. The power-supply changes and additional passive components are recommendations for this revision, not a claim that they are already fitted on existing boards. SVG files are editable vector drawings, not KiCad netlists or PCB layouts.

## Main display connection

CS=GPIO13, DC=GPIO14, RESET=GPIO2, MOSI=GPIO23, SCK=GPIO18, MISO=GPIO19, LED=GPIO4. These assignments match firmware 0.4.3. All grounds are common. Use the physical module pin numbers shown, not ESP32 chip pad numbers.

The supplied LCDWIKI circuit shows an S8050 backlight transistor and 1 kohm base resistor already on the display module. GPIO4 therefore connects directly to its LED control input. The external NPN example is only for another display exposing LEDA/LEDK; it is not installed alongside the main circuit. Its resistor calculation is an example for a specified single LED string, not a universal backlight value.

This drawing supplies the display with 3.3 V and closes the module's J1 bypass. For a module powered at 5 V, J1 must instead be open. Verify the actual module revision before changing its jumper. Touch pins and the microSD connector are intentionally unconnected; media/SD wiring is outside this revision.

## Power and startup

Rev A1 proposes AP7361C-33ER-13 in **SOT223R**, with 1=GND, 2/tab=OUT, 3=IN. The standard SOT223 AP7361C has a different pinout. This choice replaces the original unspecified L1117-33 and reduces the dropout concern after the series diode. It is not an instruction to replace a working board's regulator without checking its package and layout.

If retaining an LM1117, check its manufacturer's output-capacitor/ESR requirements and dropout at the actual load. The 100 nF input capacitor alone in the source fragment does not provide the bulk decoupling shown in this revision.

A 5 V / 1 A source is a starting requirement, not a guarantee of sufficient LDO cooling. Allow at least 500 mA for ESP32 plus display current. Check the rail during Wi-Fi transmission and calculate LDO dissipation for the real average load; provide thermal copper or use an appropriate buck supply for sustained high load. At 4.7 V input and 0.5 A load, the LDO dissipates approximately 0.7 W.

The EN network includes a 10 kohm pull-up and 1 uF capacitor. BOOT pulls GPIO0 to ground; RESET pulls EN to ground. Do not add a pull-up to GPIO2, used here for TFT reset, or force GPIO12 to a new strap state. Internal flash/PSRAM connections remain reserved.

Three optional buttons use GPIO36, GPIO39 and GPIO35 with external 10 kohm pull-ups, matching the supplied BTN_A/B/C assignment. Their menu behavior is not implemented in current firmware.

## Generic UART programmer

| J2 / original P10 | ESP32 signal | USB-UART signal direction |
| --- | --- | --- |
| 1 GND | Common ground | GND |
| 2 RX | GPIO3 / U0RXD input | Adapter TX output |
| 3 TX | GPIO1 / U0TXD output | Adapter RX input |

Use a USB-UART adapter with **3.3 V logic**; ESP LINK v1.0 is simply a tested example model. The markings on target-oriented programmer sockets may be named from the target's perspective, so label-to-label RX/RX and TX/TX can correspond to the correct electrical directions. Determine direction from the adapter documentation; do not blindly change a verified connection.

Power the board at J1 and connect the adapter's ground and two UART signals. Do not connect another supply output to the powered 3.3 V rail. BOOT and RESET are operated manually in this circuit. The example adapter's ESP-01 socket and automatic-reset wiring are not required.

## Validation status

Checked against firmware pin assignments and the cited schematics; the connectivity list accounts for all 39 module pads and rejects duplicate pin assignments. All three PDF pages were rendered and visually inspected. No KiCad ERC, SPICE simulation, PCB layout verification or hardware validation of the proposed new power circuit has been performed. Assembly validation must include polarity, 3.3 V stability, boot mode, regulator temperature and backlight current.

Regenerate using Python with ReportLab: `python generate_schematic.py`. Outputs are written alongside the source. Original manufacturer PDFs are linked rather than redistributed.

## Source documentation

- [LCDWIKI module and downloads](https://www.lcdwiki.com/2.4inch_SPI_Module_ILI9341_SKU:MSP2402)
- [LCDWIKI module schematic](https://www.lcdwiki.com/res/MSP2402/MSP2402-2.4-SPI.pdf)
- [Espressif documentation catalogue](https://www.espressif.com/en/support/documents/technical-documents)
- [WROVER-E / WROVER-IE datasheet](https://documentation.espressif.com/esp32-wrover-e_esp32-wrover-ie_datasheet_en.html)
- [ESP32 hardware design guidelines](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32/schematic-checklist.html)
- [AP7361C datasheet](https://www.diodes.com/datasheet/download/AP7361C.pdf)
- [LM1117 datasheet](https://www.ti.com/lit/ds/symlink/lm1117.pdf)
- [MMBT2222A datasheet](https://www.onsemi.com/pdf/datasheet/mmbt2222lt1-d.pdf)
- [IOT-MCU ESP LINK v1.0 repository](https://github.com/IOT-MCU/ESP-LINK-v1.0)
- [ESP LINK v1.0 user manual](https://github.com/IOT-MCU/ESP-LINK-v1.0/blob/master/ESP%20LINK%20v1.0%20usage.pdf)
