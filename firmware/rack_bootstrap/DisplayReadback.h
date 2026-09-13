#pragma once
#include <Adafruit_ILI9341.h>

// Indexed reads use Adafruit's ILI9341 D9 read-index helper. No reset,
// framebuffer read or automatic panel reconfiguration is performed here.
namespace DisplayReadback {
static constexpr int MISO_PIN = 19;
static uint8_t bytes[12]{};
static bool sampled = false, stable = false;
static uint32_t sampledAt = 0;

inline void sample(Adafruit_ILI9341 &panel) {
  const uint32_t now = millis();
  if (sampled && uint32_t(now - sampledAt) < 1000) return;
  uint8_t first[12];
  stable = true;
  for (unsigned pass = 0; pass < 2; ++pass) {
    uint8_t current[12];
    // D3: dummy (index 0), IC version, model high byte, model low byte.
    for (unsigned i = 0; i < 3; ++i)
      current[i] = panel.readcommand8(0xD3, i + 1);
    for (unsigned i = 0; i < 4; ++i)
      current[3 + i] = panel.readcommand8(ILI9341_RDDST, i + 1);
    current[7] = panel.readcommand8(ILI9341_RDMODE);
    current[8] = panel.readcommand8(ILI9341_RDMADCTL);
    current[9] = panel.readcommand8(ILI9341_RDPIXFMT);
    current[10] = panel.readcommand8(ILI9341_RDIMGFMT);
    current[11] = panel.readcommand8(ILI9341_RDSELFDIAG);
    for (unsigned i = 0; i < sizeof(current); ++i) {
      if (pass == 0) first[i] = current[i];
      else { if (first[i] != current[i]) stable = false; bytes[i] = current[i]; }
    }
  }
  sampledAt = now;
  sampled = true;
}

inline String hexBytes(unsigned start, unsigned count) {
  String result = "\"0x";
  for (unsigned i = start; i < start + count; ++i) {
    char text[3]; snprintf(text, sizeof(text), "%02X", bytes[i]); result += text;
  }
  return result + "\"";
}

inline String status() {
  bool allZero = true, allFF = true;
  for (uint8_t value : bytes) { allZero &= value == 0; allFF &= value == 255; }
  const bool responsive = sampled && stable && !allZero && !allFF;
  const char *state = !sampled ? "not_sampled" : !stable ? "unstable" :
    (allZero || allFF) ? "no_response" : "response_observed";
  String result = String("{\"miso\":19,\"state\":\"") + state +
    "\",\"sampled_at_ms\":" + String(sampledAt) + ",\"stable\":" +
    (stable ? "true" : "false") + ",\"ili9341_id_match\":" +
    ((responsive && bytes[1] == 0x93 && bytes[2] == 0x41) ? "true" : "false") +
    ",\"id_d3\":" + hexBytes(0,3) + ",\"status_09\":" + hexBytes(3,4) +
    ",\"power_0a\":" + hexBytes(7,1) + ",\"madctl_0b\":" + hexBytes(8,1) +
    ",\"pixel_format_0c\":" + hexBytes(9,1) + ",\"image_format_0d\":" + hexBytes(10,1) +
    ",\"self_diagnostic_0f\":" + hexBytes(11,1) + "}";
  return result;
}
} // namespace DisplayReadback
