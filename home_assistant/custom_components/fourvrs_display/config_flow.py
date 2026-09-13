"""Explicit MQTT handshake before adding an individual display."""
import asyncio
import json
import re
import uuid

import voluptuous as vol
from homeassistant import config_entries
from homeassistant.components import mqtt
from homeassistant.core import callback
from homeassistant.helpers import selector

from .const import DOMAIN
from .payload import decode_capabilities, validate_selection


async def handshake(hass, device_id, key):
    request_id = uuid.uuid4().hex
    future = asyncio.get_running_loop().create_future()

    @callback
    def reply(message):
        try:
            data = decode_capabilities(message.payload, device_id, request_id)
        except (ValueError, TypeError, KeyError):
            return
        if not future.done():
            future.set_result(data)

    base = f"4vrs/display/{device_id}"
    unsub = await mqtt.async_subscribe(hass, base + "/capabilities", reply, qos=0)
    try:
        for _ in range(3):
            await mqtt.async_publish(hass, base + "/request", json.dumps({"schema": 1, "key": key, "request": "hello", "request_id": request_id}), qos=0, retain=False)
            try:
                return await asyncio.wait_for(asyncio.shield(future), timeout=3)
            except TimeoutError:
                pass
        raise TimeoutError("Device did not confirm pairing key")
    finally:
        unsub()
        if not future.done():
            future.cancel()


def selection_schema(default=None):
    return vol.Schema({vol.Required("entities", default=default or []): selector.EntitySelector(selector.EntitySelectorConfig(multiple=True))})


class DisplayConfigFlow(config_entries.ConfigFlow, domain=DOMAIN):
    VERSION = 1

    async def async_step_user(self, user_input=None):
        errors = {}
        if user_input is not None:
            device_id = user_input["device_id"].strip()
            key = user_input["pairing_key"].strip()
            if not re.fullmatch(r"4vrs-rack-[0-9a-f]{12}", device_id) or not re.fullmatch(r"[0-9a-f]{32}", key):
                errors["base"] = "invalid_identity"
            elif not mqtt.is_connected(self.hass):
                errors["base"] = "mqtt_unavailable"
            else:
                await self.async_set_unique_id(device_id)
                self._abort_if_unique_id_configured()
                try:
                    await handshake(self.hass, device_id, key)
                except Exception:
                    errors["base"] = "cannot_connect"
                else:
                    self._settings = {"device_id": device_id, "pairing_key": key}
                    return await self.async_step_entities()
        return self.async_show_form(step_id="user", data_schema=vol.Schema({
            vol.Required("device_id"): str,
            vol.Required("pairing_key"): selector.TextSelector(selector.TextSelectorConfig(type=selector.TextSelectorType.PASSWORD)),
        }), errors=errors)

    async def async_step_entities(self, user_input=None):
        errors = {}
        if user_input is not None:
            try:
                entities = validate_selection(user_input["entities"])
            except ValueError:
                errors["base"] = "invalid_selection"
            else:
                return self.async_create_entry(title=self._settings["device_id"], data={**self._settings, "entities": entities})
        return self.async_show_form(step_id="entities", data_schema=selection_schema(), errors=errors)

    @staticmethod
    @callback
    def async_get_options_flow(config_entry):
        return DisplayOptionsFlow()


class DisplayOptionsFlow(config_entries.OptionsFlow):
    async def async_step_init(self, user_input=None):
        errors = {}
        if user_input is not None:
            try:
                validate_selection(user_input["entities"])
            except ValueError:
                errors["base"] = "invalid_selection"
            else:
                return self.async_create_entry(title="", data=user_input)
        current = self.config_entry.options.get("entities", self.config_entry.data["entities"])
        return self.async_show_form(step_id="init", data_schema=selection_schema(current), errors=errors)
