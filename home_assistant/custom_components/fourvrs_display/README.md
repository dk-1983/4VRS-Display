# 4VRS Display — Home Assistant integration 0.4.0

Copy this directory into `config/custom_components/fourvrs_display` and restart
Home Assistant. Configure MQTT on both Home Assistant and the display, then add
4VRS Display using the device's own Device ID and pairing key. Select 1–20 entities.

Use the integration entry's settings to edit the selection. Existing pairing is
preserved when updating the component. Firmware 0.4.3 supports the per-device
automatic firmware update switch. Entity cards display states; they do not control
physical equipment from the display.

Documentation: https://github.com/dk-1983/4VRS-Display/blob/main/docs/home-assistant.md
