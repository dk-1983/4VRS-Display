#pragma once
#include <SPI.h>
#include <Adafruit_ILI9341.h>
#include "DemoImage.h"

// NADIM V5 P6: CS=13, RESET=2, DC=14, MOSI=23, SCK=18.
// Readback/MISO is not wired. Completion means bytes sent, not LCD detection.
static constexpr uint32_t DISPLAY_SPI_HZ = 1000000;
static SPIClass displaySpi(VSPI);
static Adafruit_ILI9341 display(&displaySpi, 14, 13, 2);
static uint16_t displayRow = 0;
static uint16_t rowPixels[DEMO_WIDTH];
static bool displayStarted = false;
static_assert(DEMO_WIDTH == 240 && DEMO_HEIGHT == 320, "Expected portrait demo");
static_assert(sizeof(DEMO_BMP) == DEMO_PIXEL_OFFSET + DEMO_WIDTH * DEMO_HEIGHT * 2,
              "Expected packed RGB565 BMP");

void startDisplayDemo() {
  displaySpi.begin(18, -1, 23, 13);
  display.begin(DISPLAY_SPI_HZ);
  display.setRotation(0);
  // User reports 0x20 is upside down: flip both scan axes for a 180-degree turn.
  // Keep MV and RGB order, along with the full 240x320 logical drawing area.
  uint8_t madctl = 0xE0;
  display.sendCommand(ILI9341_MADCTL, &madctl, 1);
  display.invertDisplay(false);
  displayStarted = true;
  displayRow = 0;
}

void updateDisplayDemo() {
  if (!displayStarted || displayRow >= DEMO_HEIGHT) return;
  // Original portrait BMP, top-down RGB565 little-endian; no scaling or rotation.
  for (unsigned x = 0; x < DEMO_WIDTH; ++x) {
    const uint32_t offset = DEMO_PIXEL_OFFSET + (uint32_t(displayRow) * DEMO_WIDTH + x) * 2;
    rowPixels[x] = uint16_t(pgm_read_byte(DEMO_BMP + offset)) |
                   (uint16_t(pgm_read_byte(DEMO_BMP + offset + 1)) << 8);
  }
  display.drawRGBBitmap(0, displayRow, rowPixels, DEMO_WIDTH, 1);
  ++displayRow;
  // Return every row so the main loop can service HTTP and ArduinoOTA.
}

String displayStatus() {
  return String("{\"driver\":\"ILI9341\",\"width\":240,\"height\":320,\"rotation\":0,\"spi_hz\":") +
    String(DISPLAY_SPI_HZ) + ",\"madctl_sent\":224,\"rotation_from_0_1_10_deg\":180,\"image_rotation_cw\":0,\"image_width\":240,\"image_height\":320,\"rows_sent\":" + String(displayRow) +
    ",\"readback\":false,\"cs\":13,\"dc\":14,\"reset\":2,\"mosi\":23,\"sck\":18}";
}
