# Release notes

## Firmware 1.0.0 / Home Assistant integration 0.4.3

### English

This release combines the MQTT room display with a standalone photo frame and GIF player. Firmware and Home Assistant integration versions remain independent.

- Upload, preview and delete media through **Settings → Multimedia → Media library**. JPEG/PNG/BMP photos are converted in the browser; GIFs are uploaded unchanged.
- Play a slideshow from FAT32 SD. Set each file's inclusion, display duration (1–3600 seconds) and GIF speed (25–400%). Settings survive restart and OTA.
- Turn **Show telemetry** off to view media while MQTT keeps receiving data. A device with no selected HA entities can also operate as a photo frame.
- Skip the built-in portrait and original demonstration animation when user media exists. Keep both as fallback assets.
- Skip room covers automatically for a single room; optionally disable covers for all rooms. Room pages remain separate, with up to three cards per page and 20 selected entities overall.
- Adjust brightness in web and HA; new installations default to 50%. Startup keeps the backlight off until the first frame is ready, then fades it in.
- Change Wi-Fi using a network scan, with rollback on connection failure. Configure static IPv4 with a confirmation trial, rename the module, and read its MAC address from web or HA.
- Keep LAN ArduinoOTA, signed GitHub updates, progress on the screen, two application slots and boot validation. Existing settings are preserved by application OTA updates.
- Include optional low-motion artwork examples: BOLID equipment, a robot companion, a server rack and Tux.

**Installation:** use `4vrs-display-1.0.0.bin` for OTA. Use `4vrs-display-1.0.0-factory.bin` at address `0x0` only for initial UART installation; never send it through OTA. Install `fourvrs_display-0.4.3.zip` separately in Home Assistant and restart HA; existing pairing is retained.

**Hardware and limits:** the supported profile is ESP32-WROVER with 4 MiB PSRAM and the configured 240×320 ILI9341 panel. SD needs its own four signal connections; see the README table. FAT32 16 GB cards have been tested. GIFs must be at most 240×320 and 256 KiB; actual speed is limited by the 1 MHz display bus. The library lists up to 64 files and slideshow scanning is bounded to 128 directory entries. Video/audio, HA media uploads, physical-button navigation and local sensors are not implemented. Example animation LEDs are decorative, not live telemetry.

### Русский

Релиз объединяет MQTT-дисплей помещений с самостоятельной фоторамкой и GIF-плеером. Версии прошивки и интеграции Home Assistant независимы.

- Загрузка, предпросмотр и удаление медиа через **Настройки → Мультимедиа → Медиатека**. JPEG/PNG/BMP преобразуются в браузере, GIF загружаются без изменений.
- Слайд-шоу с FAT32 SD: выбор файлов, индивидуальное время показа (1–3600 секунд) и скорость GIF (25–400%). Настройки сохраняются после перезапуска и OTA.
- Переключатель **Показывать телеметрию** позволяет смотреть медиа, продолжая получать MQTT. Устройство без выбранных сущностей HA также работает как фоторамка.
- При наличии пользовательских медиа встроенный портрет и исходная демонстрационная анимация пропускаются; они сохранены как резервные заставки.
- Для одной комнаты заставка автоматически пропускается; все заставки комнат можно отключить вручную. Помещения не смешиваются на страницах: до трёх карточек на странице и 20 выбранных сущностей всего.
- Яркость регулируется из web и HA, начальное значение — 50%. Подсветка включается плавно после готовности первого изображения.
- Смена Wi-Fi с поиском сетей и откатом при неудаче, статический IPv4 с пробным подключением, переименование модуля и MAC-адрес в web и HA.
- Сохранены ArduinoOTA, подписанные обновления GitHub, экран прогресса, два раздела приложения и проверка запуска. OTA приложения сохраняет настройки.
- Добавлены примеры анимаций с небольшими движущимися областями: оборудование BOLID, робот, серверная стойка и Tux.

**Установка:** `4vrs-display-1.0.0.bin` — для OTA. `4vrs-display-1.0.0-factory.bin` по адресу `0x0` — только для первоначальной записи по UART, не для OTA. Архив `fourvrs_display-0.4.3.zip` устанавливается отдельно в Home Assistant с последующим перезапуском HA; сопряжение сохраняется.

**Оборудование и ограничения:** профиль ESP32-WROVER с 4 MiB PSRAM и настроенным ILI9341 240×320. Для SD отдельно распаиваются четыре сигнала — таблица есть в README. Проверены FAT32-карты на 16 ГБ. GIF: максимум 240×320 и 256 KiB; скорость ограничена шиной дисплея 1 МГц. Медиатека показывает до 64 файлов, поиск слайд-шоу ограничен 128 записями каталогов. Видео, звук, загрузка медиа через HA, навигация физическими кнопками и локальные датчики пока не реализованы. Лампочки в примерах анимаций декоративные, не телеметрические.
