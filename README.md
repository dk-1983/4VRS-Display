![4VRS Display — People, Technology, Better Together](assets/github-banner.png)

# 4VRS Display

ESP32 smart display for Home Assistant: MQTT entity cards grouped by room,
web configuration, a built-in screensaver, and signed OTA updates.

Дисплей показывает до 20 сущностей Home Assistant. Каждое помещение получает
отдельную заставку и страницы по три карточки. Значки и цвета помогают различать
типы оборудования, активные состояния и недоступные данные.

## Начать работу

- [Скачать firmware 0.4.3 и интеграцию HA 0.4.0](https://github.com/dk-1983/4VRS-Display/releases/tag/firmware-v0.4.3)
- [Подключение и первый запуск](docs/user-guide.md)
- [Поддерживаемая плата и распиновка](docs/hardware.md)
- [Настройка Home Assistant](docs/home-assistant.md)
- [Обновления](docs/updates.md)

Web-интерфейс поддерживает English (по умолчанию) и Русский. Первоначальный
web-вход: **admin / admin**; пароль меняется в Settings. Подключение к MQTT
настраивается в браузере по IP устройства.

## Возможности

- Карточки состояний, значки сущностей и группировка по помещениям.
- Встроенная статичная заставка без карты памяти.
- Настройка Wi-Fi, MQTT, языка и паролей через web.
- ArduinoOTA по LAN и подписанные обновления с GitHub с двумя слотами и откатом.
- Индивидуальное разрешение автообновлений через web и интеграцию HA.

Текущий профиль: ESP32-WROVER с 4 MiB PSRAM, NADIM V5, SPI ILI9341 240×320.
Профиль не универсален для любых плат ESP32 или дисплеев.

GIF, воспроизведение медиа с SD, загрузка фотографий через web и локальные датчики
пока не реализованы. [План развития](docs/plan.md).

## Разработка

Исходники прошивки — `firmware/`, интеграции HA — `home_assistant/`,
контракт обмена — `protocol/`. [Сборка и проверки](docs/development.md).

Собственный код — [MIT](LICENSE). Сторонние библиотеки сохраняют свои лицензии.
