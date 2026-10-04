"""Confirmed backlight percentage shared with the display web interface."""
from homeassistant.components.number import NumberEntity, NumberMode
from homeassistant.helpers.entity import DeviceInfo, EntityCategory
from homeassistant.exceptions import HomeAssistantError
from .const import DOMAIN

async def async_setup_entry(hass, entry, async_add_entities):
    async_add_entities([BacklightNumber(entry.runtime_data)])

class BacklightNumber(NumberEntity):
    _attr_should_poll = False
    _attr_has_entity_name = True
    _attr_translation_key = "backlight_brightness"
    _attr_entity_category = EntityCategory.CONFIG
    _attr_icon = "mdi:brightness-6"
    _attr_native_min_value = 0
    _attr_native_max_value = 100
    _attr_native_step = 1
    _attr_native_unit_of_measurement = "%"
    _attr_mode = NumberMode.SLIDER

    def __init__(self, runtime):
        self.runtime = runtime
        self._attr_unique_id = f"{runtime.device_id}_backlight_brightness"
        self._attr_device_info = DeviceInfo(identifiers={(DOMAIN, runtime.device_id)})

    @property
    def available(self):
        return self.runtime.backlight_supported and self.runtime.brightness is not None and self.runtime.status not in ("offline", "stale", "waiting")

    @property
    def native_value(self):
        return self.runtime.brightness

    async def async_set_native_value(self, value):
        try:
            await self.runtime.set_brightness(value)
        except (ValueError, ConnectionError, TimeoutError) as error:
            raise HomeAssistantError(str(error)) from error

    async def async_added_to_hass(self):
        await super().async_added_to_hass()
        self.runtime.listeners.add(self.async_write_ha_state)
        self.async_on_remove(lambda: self.runtime.listeners.discard(self.async_write_ha_state))
