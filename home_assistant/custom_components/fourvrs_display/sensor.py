"""Transport receipt is separate from visual verification of the physical TFT."""
from homeassistant.components.sensor import SensorEntity
from homeassistant.helpers.entity import DeviceInfo, EntityCategory
from .const import DOMAIN


async def async_setup_entry(hass, entry, async_add_entities):
    async_add_entities([DeliverySensor(entry.runtime_data)])


class DeliverySensor(SensorEntity):
    _attr_should_poll = False
    _attr_has_entity_name = True
    _attr_name = "Доставка данных"
    _attr_entity_category = EntityCategory.DIAGNOSTIC
    _attr_icon = "mdi:monitor-dashboard"

    def __init__(self, runtime):
        self.runtime = runtime
        self._attr_unique_id = f"{runtime.device_id}_delivery"
        self._attr_device_info = DeviceInfo(identifiers={(DOMAIN, runtime.device_id)}, name=runtime.device_id, manufacturer="4VRS", model="ESP32 TFT 240x320")

    @property
    def native_value(self):
        return self.runtime.status

    @property
    def extra_state_attributes(self):
        return {"sent_sequence": self.runtime.seq, "accepted_sequence": self.runtime.ack_seq, "firmware": self.runtime.firmware, "entity_count": len(self.runtime.entities)}

    async def async_added_to_hass(self):
        await super().async_added_to_hass()
        self.runtime.listeners.add(self.async_write_ha_state)
        self.async_on_remove(lambda: self.runtime.listeners.discard(self.async_write_ha_state))
