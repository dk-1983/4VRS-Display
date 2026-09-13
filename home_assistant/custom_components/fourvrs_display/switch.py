"""Confirmed, per-display automatic firmware update policy."""
from homeassistant.components.switch import SwitchEntity
from homeassistant.helpers.entity import DeviceInfo, EntityCategory
from .const import DOMAIN

async def async_setup_entry(hass, entry, async_add_entities):
    async_add_entities([AutoUpdateSwitch(entry.runtime_data)])

class AutoUpdateSwitch(SwitchEntity):
    _attr_should_poll=False
    _attr_has_entity_name=True
    _attr_translation_key="auto_update"
    _attr_entity_category=EntityCategory.CONFIG
    _attr_icon="mdi:update"
    def __init__(self,runtime):
        self.runtime=runtime
        self._attr_unique_id=f"{runtime.device_id}_auto_update"
        self._attr_device_info=DeviceInfo(identifiers={(DOMAIN,runtime.device_id)})
    @property
    def available(self):
        return self.runtime.update_supported and bool(self.runtime.update_status) and self.runtime.status not in ("offline","stale","waiting")
    @property
    def is_on(self):
        return self.runtime.update_status.get("ha_enabled")
    @property
    def extra_state_attributes(self):
        return {**self.runtime.update_status,"desired_enabled":self.runtime.entry.options.get("auto_update_enabled",True),"desired_revision":self.runtime.entry.options.get("update_policy_rev",0)}
    async def set_policy(self,enabled):
        entry=self.runtime.entry
        revision=max(entry.options.get("update_policy_rev",0),self.runtime.update_status.get("revision",0))+1
        self.hass.config_entries.async_update_entry(entry,options={**entry.options,"auto_update_enabled":enabled,"update_policy_rev":revision})
    async def async_turn_on(self,**kwargs):await self.set_policy(True)
    async def async_turn_off(self,**kwargs):await self.set_policy(False)
    async def async_added_to_hass(self):
        await super().async_added_to_hass()
        self.runtime.listeners.add(self.async_write_ha_state)
        self.async_on_remove(lambda:self.runtime.listeners.discard(self.async_write_ha_state))
