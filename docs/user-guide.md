# User guide / Руководство пользователя

Firmware **1.0.0** · Home Assistant integration **0.4.3**

## First setup

1. For a new device, flash `4vrs-display-1.0.0-factory.bin` at **0x0** using an ESP32-compatible UART tool. Factory flashing replaces the image region; use OTA for configured devices. Reset normally after successful verification.
2. Join `4vrs-rack-<id>-setup`, password `KIaE18TTn4Omp8H-0peXAk1i`, and open **http://192.168.4.1/**. Select your 2.4 GHz Wi-Fi and enter its password.
3. Find the device IP in your router. Sign in to its web interface with initial **admin / admin**, then change these credentials under **Settings → Access and language**. English is the default; Russian is selectable.
4. For telemetry, configure the MQTT broker and pair the [Home Assistant integration](home-assistant.md). Pairing with zero entities is supported. For standalone media, MQTT and HA are optional.

The setup access point turns off after stable LAN connectivity. Wi-Fi, web and ArduinoOTA credentials are separate and persist through updates. The About page shows the station MAC for DHCP reservations, firmware version and display diagnostics.

## Network and display

**Settings → Network → Wi-Fi** scans nearby networks and lets you switch networks without reflashing. The module tries the new credentials after restart. If it cannot obtain an IP within 60 seconds, it restarts with the previous network. If neither network is available, setup Wi-Fi provides recovery. The IP may change.

IPv4 can use DHCP or a static address, mask, gateway and DNS. A new static configuration must be confirmed through its LAN address within three minutes or it rolls back. Static settings are retained when changing Wi-Fi: switch to DHCP first if the new network uses another subnet.

**Settings → Device** changes the module name without changing its MQTT ID. **Display** controls brightness, room covers and **Show telemetry**. Initial brightness is 50% if no user value exists; 0 turns the backlight off. Room covers are always skipped for one room. With several rooms, the cover switch is respected; each page contains only one room's entities.

Turning telemetry off preserves MQTT and entity selection, but shows media. An empty entity list also allows media mode. Returning to available telemetry exits automatic media playback. Explicit media previews remain available.

## Media library

Open **Settings → Multimedia → Media library**. A FAT32 SD card is required for custom media; 16 GB cards have been tested. Wire the module's separate SD connector as described in [hardware](hardware.md). The firmware never formats the card.

- Photos: JPEG, PNG and BMP are converted in your browser to a 240×320 24-bit BMP. Proportions are preserved with black margins. The original photo is not uploaded. Browser input limit: 20 MB / 25 megapixels.
- GIF: uploaded unchanged, maximum **240×320 and 256 KiB**. Smaller animations are centered. Complex full-frame animations run slower than small blinking or moving regions at the fixed 1 MHz SPI clock.
- Names: Latin letters, digits, hyphens and underscores; existing names are never overwritten. Photos live in `/4vrs/photos/`, animations in `/4vrs/animations/`.
- Upload, preview, show and delete are available per file. Stop playback before deleting the currently displayed file. The library displays up to 64 files.
- Uploads use temporary files and become visible after completion and header checks. Cancellation removes the temporary upload; a power loss can leave a `.part` file in `/4vrs/cache/`. Header checks cannot guarantee decoding of every third-party GIF.

Each file has **Show for** (1–3600 seconds) and **Include in slideshow**. GIFs also have **Speed** (25–400%; 100% uses the encoded pauses). Save stores these preferences on SD and applies them immediately. Display transfer time still limits actual speed.

Enable **Slideshow** to cycle included files in filename order. Timing begins when the first frame has finished drawing; switching may occur midway through a GIF loop. With slideshow disabled, the selected file remains displayed. **Stop playback** pauses for the current session. Selecting a file or enabling slideshow resumes it. Slideshow and selected media survive restart and OTA. Up to 128 directory entries are scanned when choosing the next file.

The built-in portrait and `4vrs-test.gif` are demonstrations for devices without user media. They are excluded from automatic presentation when supported custom files exist, including at startup. Manual demo preview remains available. If all custom files are unchecked, the demo is not substituted; a stopped or empty media selection can leave a blank screen.

Video/audio, HA-side media upload and physical-button navigation are not implemented. Decorative animation lights do not represent live telemetry.

## Updates

Use `4vrs-display-1.0.0.bin` for OTA, never the factory image. The display shows update progress. LAN ArduinoOTA and signed GitHub updates remain available during media playback. See [update behavior and per-device blocking](updates.md).

## Первый запуск

Для новой платы запишите `4vrs-display-1.0.0-factory.bin` через UART по адресу **0x0** и перезапустите без BOOT. Для настроенной платы используйте OTA. Подключитесь к `4vrs-rack-<id>-setup`, пароль `KIaE18TTn4Omp8H-0peXAk1i`, откройте **http://192.168.4.1/** и выберите Wi-Fi 2,4 ГГц.

Найдите IP в роутере. Начальный web-вход **admin / admin**; смена пароля и языка — **Настройки → Доступ и язык**. По умолчанию английский. Для телеметрии настройте брокер MQTT и интеграцию HA; список сущностей может быть пустым. Для фоторамки HA и MQTT не обязательны. MAC станции, версия и диагностика дисплея находятся в **О модуле**.

## Сеть и экран

**Настройки → Сеть → Wi-Fi** позволяет найти и выбрать другую сеть. После перезапуска новая сеть проверяется 60 секунд; если IP не получен, возвращаются прежние настройки. При недоступности обеих сетей остаётся точка настройки. Новый IP может отличаться.

Для статического IPv4 задаются адрес, маска, шлюз и DNS. Новые параметры нужно подтвердить через LAN за три минуты, иначе произойдёт откат. При смене Wi-Fi статические параметры сохраняются; для другой подсети сначала включите DHCP.

Имя модуля меняется в **Устройство**, MQTT ID не меняется. В **Экран** находятся яркость, заставки комнат и **Показывать телеметрию**. Начальная яркость — 50%, если значение ещё не сохранено; 0 выключает подсветку. При одной комнате заставка всегда пропускается, при нескольких действует переключатель. Комнаты на страницах не смешиваются.

Выключение телеметрии сохраняет MQTT и сущности, но показывает медиа. Пустой список сущностей также включает медиарежим. При возвращении доступной телеметрии автоматический медиапоказ завершается; ручной предпросмотр доступен отдельно.

## Медиатека

Откройте **Настройки → Мультимедиа → Медиатека**. Нужна FAT32-карта; проверены 16 ГБ. Отдельные контакты SD подключаются по [схеме](hardware.md). Прошивка карту не форматирует.

Фото JPEG/PNG/BMP преобразуются браузером в BMP 240×320 с сохранением пропорций и чёрными полями. Исходное фото не загружается; предел исходника — 20 МБ / 25 мегапикселей. GIF передаётся без изменений: **до 240×320 и 256 КиБ**. На SPI 1 МГц лучше работают анимации с небольшими меняющимися участками.

Имена — латиница, цифры, дефисы, подчёркивания. Фото сохраняются в `/4vrs/photos/`, GIF — в `/4vrs/animations/`. Совпадающие имена не перезаписываются. Доступны загрузка, предпросмотр, показ и удаление; текущий файл перед удалением нужно остановить. Список показывает до 64 файлов.

Незавершённая загрузка не появляется в списке. Отмена удаляет временный файл; после обрыва питания в `/4vrs/cache/` может остаться `.part`. Проверка заголовка не гарантирует декодирование любого стороннего GIF.

Для каждого файла задаются **время показа** (1–3600 секунд), **участие в слайд-шоу**, для GIF ещё **скорость** (25–400%; 100% — исходные паузы). Настройки сохраняются на SD и применяются сразу. Реальная скорость ограничена передачей пикселей на экран.

**Слайд-шоу** чередует отмеченные файлы по имени. Время отсчитывается после отрисовки первого кадра, переход возможен посреди GIF-цикла. Без слайд-шоу выбранный файл показывается постоянно. **Остановить просмотр** приостанавливает показ до запуска файла, включения слайд-шоу или перезапуска. Выбранное медиа и режим переживают OTA. При выборе следующего файла проверяется до 128 записей каталогов.

Встроенный портрет и `4vrs-test.gif` — демонстрации для устройства без собственных изображений. При наличии поддерживаемых пользовательских файлов они не выбираются автоматически, в том числе при запуске. Ручной тест демо доступен. Если снять все свои файлы со слайд-шоу, демка не подставляется вместо них; остановленный или пустой медиапоказ может оставлять чёрный экран.

Видео, звук, загрузка медиа из HA и физические кнопки пока не реализованы. Мигающие лампочки в заставках — оформление, а не показания реальных устройств.

## Обновление

Для OTA используется `4vrs-display-1.0.0.bin`, не factory-образ. На дисплее показывается прогресс. ArduinoOTA и подписанные обновления GitHub доступны при воспроизведении. [Блокировка автообновлений и проверка запуска](updates.md).
