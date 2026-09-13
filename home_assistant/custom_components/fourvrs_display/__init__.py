"""4VRS Display MQTT bridge: first three-entity milestone."""
from .const import PLATFORMS
from .runtime import DisplayRuntime


async def async_setup_entry(hass, entry):
    runtime = DisplayRuntime(hass, entry)
    entry.runtime_data = runtime
    try:
        await runtime.start()
        await hass.config_entries.async_forward_entry_setups(entry, PLATFORMS)
    except Exception:
        runtime.stop()
        raise
    entry.async_on_unload(entry.add_update_listener(_reload))
    return True


async def _reload(hass, entry):
    await hass.config_entries.async_reload(entry.entry_id)


async def async_unload_entry(hass, entry):
    if await hass.config_entries.async_unload_platforms(entry, PLATFORMS):
        entry.runtime_data.stop()
        return True
    return False
