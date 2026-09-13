"""Resolve selected entities against HA registries on each snapshot."""


def resolve_areas(hass, entities):
    # Lazy imports keep the wire encoder and older-session tests independent of HA.
    from homeassistant.helpers import area_registry as ar, device_registry as dr, entity_registry as er

    entity_registry = er.async_get(hass)
    area_registry = ar.async_get(hass)
    result = {}
    for entity_id in entities:
        entry = entity_registry.async_get(entity_id)
        area_id = None
        if entry is not None:
            if hasattr(er, "async_get_effective_area_id"):
                area_id = er.async_get_effective_area_id(hass, entry)
            else:
                # Compatibility with HA versions before the effective-area helper.
                area_id = entry.area_id
                if area_id is None and entry.device_id:
                    device = dr.async_get(hass).async_get(entry.device_id)
                    if device is not None:
                        area_id = dr.async_get_effective_area_id(hass, device) if hasattr(dr, "async_get_effective_area_id") else device.area_id
        area = area_registry.async_get_area(area_id) if area_id else None
        result[entity_id] = (area.id, area.name) if area is not None else ("", "")
    return result
