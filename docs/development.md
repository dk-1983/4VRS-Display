# Сборка и разработка

## Структура

- `firmware/rack_bootstrap/` — приложение Arduino-ESP32.
- `home_assistant/custom_components/fourvrs_display/` — интеграция HA.
- `protocol/` — JSON Schema и примеры MQTT.
- `tools/` — сборка, подготовка изображений, OTA и подпись релизов.
- `assets/` — изображения; `docs/` — публичные руководства.

## Проверки

```sh
python -m unittest discover -s home_assistant/tests -v
python -m pip install -r tools/requirements-release.txt
python tools/test_release.py
```

Сборка использует Arduino CLI, установленный core `esp32:esp32@3.3.8` и
библиотеки Adafruit GFX / Adafruit ILI9341 с их зависимостями. Параметры профиля
зафиксированы в `tools/build.py`.

```sh
python tools/build.py --config <arduino-cli.yaml> --version 0.4.3
```

Путь к Arduino CLI можно передать через `--cli`, каталог результата — через
`--output`. При росте MQTT-буферов проверяйте стек с `tools/check_mqtt_stack.py`.
На WROVER крупные входящие буферы размещены в PSRAM, чтобы оставить внутреннюю
память для TLS. Сохранение Wi-Fi, двух OTA-слотов и обслуживания OTA обязательно.

## Выпуск

`tools/release.py` создаёт подписанный манифест и бинарник из публичной сборки.
Ключ подписи и индивидуальные реквизиты хранятся вне Git. Приватные миграционные,
тестовые и несовместимые образы отклоняются. После испытаний опубликованного
кандидата манифест переносится в `releases/stable.json`. Не заменяйте бинарники
уже выпущенных версий. Локальные журналы и инструкции агенту не являются публичной
документацией и исключены из отслеживания.
