# Home Assistant

## Установка и сопряжение

1. Настройте штатную интеграцию MQTT в HA и тот же брокер на дисплее.
2. Скачайте `fourvrs_display-0.4.3.zip` из релиза. Поместите каталог
   `custom_components/fourvrs_display` в `config/custom_components/` HA.
3. Перезапустите Home Assistant и добавьте интеграцию **4VRS Display**.
4. Укажите Device ID и pairing key из защищённой страницы MQTT именно этого прибора.
5. Выберите от 0 до 20 различных сущностей и сохраните настройки.

При обновлении компонента существующее сопряжение сохраняется. Для изменения
списка откройте Settings → Devices & services → Integrations → 4VRS Display,
затем шестерёнку записи интеграции. Карандаш карточки устройства изменяет его
метаданные, а не список передаваемых сущностей.

## Отображение

Принадлежность к помещению берётся сначала у сущности, затем у её устройства.
Неназначенные сущности образуют отдельную группу. Между комнатами показывается
заставка с названием и значком на 3 секунды, страницы по три карточки — на 8 секунд.
Комнаты не смешиваются: пять сущностей дают страницы 3+2.

Тип и device_class помогают выбрать значок: термометр, капля с процентом для
влажности, лампа, вентилятор, клапан и другие семейства. Сухой датчик протечки
отображается перечёркнутой каплей. `unavailable`, `unknown` и устаревшие данные
не считаются состоянием «выключено» или «сухо». Неизвестные типы имеют текстовый
fallback. Произвольные MDI-значки и управление физическим оборудованием с экрана
в этом релизе не реализованы.

Снимки отправляются при изменении состояний и каждые 30 секунд. Через 90 секунд
без свежего снимка данные помечаются устаревшими. Для каждого дисплея настраивается
собственный список; ключ сопряжения не следует переносить между приборами.

Переключатель **Автообновление** управляет разрешением установки firmware.
Его взаимодействие с web-блокировкой описано в [руководстве обновлений](updates.md).

## Backlight brightness — firmware 0.4.5 / integration 0.4.1

In the device configuration, **Backlight brightness** sets 0–100%; 0 turns the
backlight off. The same control is available in the device web **Settings**.
The display saves the latest change from either interface and restores it after
a restart or OTA. HA shows the value confirmed by the display over MQTT; old
firmware without this capability leaves the control unavailable. Install the
updated custom component and restart Home Assistant to add this entity.

The supported GPIO4 backlight stays dark while the first image is drawn, fades
to the saved level over 1.2 seconds, then holds the built-in startup image for
three more seconds before displaying room data when no user media exists. With user media, the built-in startup picture is skipped.

## Яркость подсветки — прошивка 0.4.5 / интеграция 0.4.1

В конфигурации устройства **Яркость подсветки** задаёт уровень 0–100%; 0 выключает
подсветку. Такой же регулятор находится в web-разделе **Настройки** дисплея.
Последнее изменение из HA или web сохраняется на плате и восстанавливается после
перезапуска и OTA. HA показывает подтверждённое платой значение через MQTT.
На старой прошивке без этой функции регулятор недоступен. Для появления сущности
обновите custom component и перезапустите Home Assistant.

На поддерживаемом профиле GPIO4 подсветка включается после готовности изображения,
плавно выходит на сохранённый уровень за 1,2 секунды, затем стартовая картинка
остаётся ещё на 3 секунды перед показом данных помещений.

## Wi-Fi MAC address / MAC-адрес Wi-Fi

Firmware 0.4.6 exposes the station MAC in Web → About and `/health` as `mac_address`. HA integration 0.4.2 adds a diagnostic **Wi-Fi MAC address** sensor on each display device. Use this address for DHCP reservations. Older firmware leaves the sensor unknown.

Прошивка 0.4.6 показывает MAC станции Wi-Fi в Web → О модуле. Интеграция HA 0.4.2 добавляет диагностический датчик **MAC-адрес Wi-Fi** для каждой платы. Это адрес для резервирования IP в DHCP, не MAC точки настройки. На старой прошивке значение неизвестно.

## Optional entities / Необязательные сущности

Integration 0.4.3 allows pairing with no selected entities and warns that only Photo & Multimedia is available. Add entities later in integration options. Firmware 1.0.0 accepts an empty snapshot to clear previous cards and use selected media, or the built-in artwork if no custom media exists. Older firmware can pair without entities but does not receive empty snapshots; previously displayed cards may remain until restart.

Интеграция 0.4.3 разрешает сопряжение без сущностей с предупреждением о режиме Photo & Multimedia. Сущности добавляются позже в настройках интеграции. Прошивка 1.0.0 при очистке списка убирает прежние карточки и показывает выбранное медиа либо встроенную картинку, если пользовательских файлов нет. Старой прошивке пустые снимки не отправляются; прежние карточки могут оставаться до перезапуска.
