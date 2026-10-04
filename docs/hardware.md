# Оборудование

Профиль релиза: ESP32-WROVER с 4 MiB PSRAM, плата NADIM V5 и SPI TFT ILI9341
240×320 в портретной ориентации. Разметка использует 4 MiB flash с двумя слотами
приложения по 0x1E0000; большие микросхемы flash также могут использовать этот профиль.

| Сигнал TFT | GPIO ESP32 |
|---|---:|
| CS | 13 |
| DC | 14 |
| RESET | 2 |
| MOSI / SDI | 23 |
| SCK | 18 |
| MISO / SDO | 19 |
| Управление подсветкой | 4 |

GPIO4 подключается напрямую к входу LED нашего модуля. По схеме LCDWIKI
на модуле уже есть S8050 и резистор базы 1 кОм; ток светодиодов через GPIO не идёт.
Внешний транзистор нужен только для другого типа дисплея с отдельными LEDA/LEDK.
Логические уровни ESP32 — 3,3 В. Питание TFT выбирается по схеме конкретного модуля.
Наличие или отсутствие контроллера touch не определяет контроллер изображения.

SPI: 1 МГц, MADCTL 0xE0, инверсия выключена. Эти параметры относятся к данному
профилю; другая матрица или разводка может потребовать изменения прошивки.

## UART

Используйте совместимый USB-UART с логическими уровнями 3,3 В и общим GND.
Соединяются выход TX одной стороны и вход RX другой: ориентируйтесь на направления
сигналов в схемах, поскольку маркировка разъёмов адаптеров бывает разной.
BOOT/PGM удерживается при RESET для входа в загрузчик. Лог приложения — 115200 бод.

## Диагностика

`/health` показывает версию, состояние сети, размер PSRAM и подтверждение запуска.
`/display` показывает параметры изображения и ответы регистров при подключённом
SDO/MISO. Нули при отсутствующем MISO не подтверждают неисправность дисплея.
Чтение D3 ID в текущем профиле может возвращать нули при работающем изображении;
это ограничение диагностики. Ответы отдельных регистров не заменяют визуальную
проверку ориентации, цветов и заполнения экрана.

## Электрическая схема и документация

Базовый вариант проекта — исходная схема автора на L1117-33, по которой
собраны работающие устройства.

[Расширенный вариант A1: PDF/SVG, перечень деталей и UART](../hardware/schematic/README.ru.md)
содержит необязательные изменения питания и обвязки. Автор проверил схему A1;
она не заменяет базовую реализацию и не требует переделки рабочих плат.
Кнопки предусмотрены на перспективу и пока не используются прошивкой.

- [LCDWIKI: модуль SPI ILI9341 2,4 дюйма](https://www.lcdwiki.com/2.4inch_SPI_Module_ILI9341_SKU:MSP2402) — электрическая схема, руководство и габаритные чертежи.
- [Каталог документации Espressif](https://www.espressif.com/en/support/documents/technical-documents) — datasheet нужно выбирать по точной маркировке модуля.
- [Datasheet ESP32-WROVER-E / WROVER-IE](https://documentation.espressif.com/esp32-wrover-e_esp32-wrover-ie_datasheet_en.html) и [рекомендации Espressif по схемотехнике ESP32](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32/schematic-checklist.html).
- [Документация ESP LINK v1.0 от IOT-MCU](https://github.com/IOT-MCU/ESP-LINK-v1.0) — пример USB-UART адаптера, а не обязательная модель. Инструкции для ESP-01 из его руководства не заменяют порядок прошивки ESP32.
- [Наша электрическая схема, перечень деталей и пояснения](../hardware/schematic/README.ru.md).

## SD card SPI wiring / Подключение SD

Reserve GPIO25 for SD_CS. Connect the separate SD header on the display module: SD_MOSI → GPIO23, SD_MISO → GPIO19, SD_CLK → GPIO18, SD_CS → GPIO25. TFT_CS remains GPIO13. The SD header is not internally connected to the TFT SPI header. Firmware 1.0.0 mounts the card at startup and plays BMP/GIF media. Status is available under Settings → Multimedia → SD card. Formatting is never performed; 16 GB FAT32 cards have been tested on assembled hardware. Shared MISO uses a pull-up; do not retain the previous TFT-only pull-down.

Для SD_CS выделен GPIO25 (контакт «25» на NADIM V5). SD_MOSI → GPIO23, SD_MISO → GPIO19, SD_CLK → GPIO18. CS дисплея остаётся GPIO13. На исходной радиоплате GPIO25 обозначен также PCM-LRCK; это назначение не используется прошивкой 4VRS Display. В 1.0.0 карта монтируется при запуске и используется для фото BMP и GIF. Состояние: Настройки → Мультимедиа → SD-карта. Форматирование не выполняется; проверены FAT32-карты 16 ГБ на собранных платах. Общая линия MISO использует подтяжку вверх вместо прежней подтяжки вниз для отдельного TFT.
