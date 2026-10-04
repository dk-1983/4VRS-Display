"""One display session: state subscriptions, bounded coalescing, heartbeat and ACK."""
import asyncio
from datetime import timedelta
import json
import re
import time
import uuid

from homeassistant.components import mqtt
from homeassistant.core import callback
from homeassistant.helpers.event import async_call_later, async_track_state_change_event, async_track_time_interval

from .room_icons import room_icon
from .areas import resolve_areas
from .presentation import presentation
from .payload import decode_capabilities, encode_snapshot, validate_selection


class DisplayRuntime:
    def __init__(self, hass, entry):
        self.hass, self.entry = hass, entry
        self.device_id = entry.data["device_id"]
        self.key = entry.data["pairing_key"]
        self.entities = validate_selection(entry.options.get("entities", entry.data["entities"]))
        self.base = f"4vrs/display/{self.device_id}"
        self.session = None
        self.empty_snapshot_supported = False
        self.presentation_supported = False
        self.areas_supported = False
        self.area_icons_supported = False
        self.update_supported = False
        self.update_status = {}
        self.backlight_supported = False
        self.brightness = None
        self.backlight_seq = 0
        self.backlight_pending = {}
        self.seq = self.ack_seq = 0
        self.last_ack = self.last_hello = 0.0
        self.waiting_since = time.monotonic()
        self.request_id = uuid.uuid4().hex
        self.status = "waiting"
        self.firmware = None
        self.mac_address = None
        self.unsubs, self.listeners = [], set()
        self.pending = None
        self.lock = asyncio.Lock()
        self.closed = False

    @callback
    def changed(self):
        for listener in tuple(self.listeners):
            listener()

    async def start(self):
        try:
            self.unsubs.append(await mqtt.async_subscribe(self.hass, self.base + "/capabilities", self.capabilities, qos=0))
            self.unsubs.append(await mqtt.async_subscribe(self.hass, self.base + "/ack", self.ack, qos=0))
            self.unsubs.append(await mqtt.async_subscribe(self.hass, self.base + "/update/status", self.receive_update_status, qos=0))
            self.unsubs.append(await mqtt.async_subscribe(self.hass, self.base + "/backlight/status", self.receive_backlight, qos=0))
            self.unsubs.append(await mqtt.async_subscribe(self.hass, self.base + "/availability", self.availability, qos=0))
            self.unsubs.append(async_track_state_change_event(self.hass, self.entities, self.state_changed))
            self.unsubs.append(async_track_time_interval(self.hass, self.heartbeat, timedelta(seconds=30)))
            await self.hello()
        except Exception:
            self.stop()
            raise

    async def hello(self):
        if self.closed or not mqtt.is_connected(self.hass):
            return
        self.last_hello = time.monotonic()
        await mqtt.async_publish(self.hass, self.base + "/request", json.dumps({"schema": 1, "key": self.key, "request": "hello", "request_id": self.request_id}), qos=0, retain=False)

    async def capabilities(self, message):
        if self.closed:
            return
        try:
            caps = decode_capabilities(message.payload, self.device_id)
        except (ValueError, TypeError, KeyError):
            return
        if caps.get("request_id") == self.request_id:
            if caps["session"] != self.session:
                self.update_status = {}
                self.brightness = None
                self.backlight_seq = 0
                self.cancel_backlight()
                self.session = caps["session"]
                self.seq = self.ack_seq = 0
                self.last_ack = 0.0
                self.waiting_since = time.monotonic()
                self.status = "sending"
            mac = caps.get("mac_address")
            self.mac_address = mac.upper() if isinstance(mac, str) and re.fullmatch(r"(?:[0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}", mac) else None
            self.empty_snapshot_supported = caps.get("empty_snapshot_v") == 1
            self.presentation_supported = caps.get("presentation_v") == 1
            self.areas_supported = caps.get("area_v") == 1 and type(caps.get("max_payload")) is int and caps["max_payload"] >= 15360
            self.area_icons_supported = self.areas_supported and caps.get("area_icon_v") == 1
            self.update_supported = caps.get("update_v") == 1
            self.backlight_supported = caps.get("backlight_v") == 1
            level = caps.get("brightness_percent")
            if self.backlight_supported and type(level) is int and 0 <= level <= 100:
                self.brightness = level
            self.changed()
            self.firmware = caps.get("firmware")
            if not self.entities and not self.empty_snapshot_supported:
                self.status = "idle"
                self.last_ack = time.monotonic()
                self.changed()
            await self.publish_policy()
            await self.publish()
        elif caps["session"] != self.session and time.monotonic() - self.last_hello > 2:
            await self.hello()

    @callback
    def ack(self, message):
        if self.closed or not isinstance(message.payload, str) or len(message.payload) > 1024:
            return
        try:
            data = json.loads(message.payload)
        except ValueError:
            return
        if not isinstance(data, dict) or data.get("schema") != 1 or data.get("session") != self.session:
            return
        seq = data.get("seq")
        if data.get("status") == "accepted" and type(seq) is int and self.ack_seq < seq <= self.seq:
            self.ack_seq = seq
            self.last_ack = time.monotonic()
            self.status = "accepted"
        elif data.get("status") == "rejected":
            self.status = "rejected"
        self.changed()

    @callback
    def availability(self, message):
        if message.payload == "offline":
            self.status = "offline"
            self.session = None
            self.brightness = None
            self.cancel_backlight()
            self.changed()

    @callback
    def state_changed(self, event):
        if self.closed or self.pending is not None:
            return
        self.pending = async_call_later(self.hass, 0.25, self.flush)

    async def flush(self, _):
        self.pending = None
        await self.publish()

    async def heartbeat(self, _):
        await self.publish_policy()
        if not self.entities and not self.empty_snapshot_supported and self.session:
            if time.monotonic() - self.last_ack > 90:
                self.status = "stale"
                self.changed()
            await self.hello()
            return
        if self.closed:
            return
        if not self.session or self.status in ("offline", "waiting"):
            await self.hello()
        else:
            if time.monotonic() - (self.last_ack or self.waiting_since) > 90:
                self.status = "stale"
                self.changed()
            await self.publish()

    @callback
    def receive_update_status(self, message):
        if self.closed or not isinstance(message.payload,str) or len(message.payload)>2048:
            return
        try:
            data=json.loads(message.payload)
        except ValueError:
            return
        if isinstance(data,dict) and self.session and data.get("schema")==1 and data.get("device_id")==self.device_id and data.get("session")==self.session and type(data.get("ha_enabled")) is bool and type(data.get("revision")) is int and 0<=data["revision"]<=0x7fffffff:
            self.update_status=data
            self.changed()

    def cancel_backlight(self):
        for future in self.backlight_pending.values():
            if not future.done():
                future.set_exception(ConnectionError("Display disconnected"))
        self.backlight_pending.clear()

    @callback
    def receive_backlight(self, message):
        if self.closed or not isinstance(message.payload, str) or len(message.payload) > 1024:
            return
        try:
            data = json.loads(message.payload)
        except ValueError:
            return
        if not isinstance(data, dict) or data.get("schema") != 1 or not self.session or data.get("session") != self.session or data.get("device_id") != self.device_id:
            return
        level, seq, result = data.get("brightness_percent"), data.get("command_seq"), data.get("result")
        if type(level) is not int or not 0 <= level <= 100 or type(seq) is not int or not 0 <= seq <= 0xffffffff or result not in ("state", "accepted", "stale_command", "storage_error"):
            return
        self.brightness = level
        future = self.backlight_pending.get(seq)
        if future and not future.done():
            if result == "accepted":
                future.set_result(level)
            elif result != "state":
                future.set_exception(ValueError("Display rejected brightness: " + result))
        self.changed()

    async def set_brightness(self, value):
        if type(value) not in (int, float) or not 0 <= value <= 100 or int(value) != value:
            raise ValueError("Brightness must be an integer from 0 to 100")
        if self.closed or not self.session or not self.backlight_supported or self.status in ("offline", "stale", "waiting") or not mqtt.is_connected(self.hass):
            raise ConnectionError("Display brightness control is unavailable")
        if self.backlight_seq >= 0xffffffff:
            raise ValueError("Reconnect the display session before sending more commands")
        self.backlight_seq += 1
        seq = self.backlight_seq
        future = asyncio.get_running_loop().create_future()
        self.backlight_pending[seq] = future
        try:
            message = {"schema": 1, "key": self.key, "session": self.session, "request": "backlight", "command_seq": seq, "brightness_percent": int(value)}
            await mqtt.async_publish(self.hass, self.base + "/request", json.dumps(message), qos=0, retain=False)
            await asyncio.wait_for(future, timeout=8)
        finally:
            self.backlight_pending.pop(seq, None)
            if not future.done():
                future.cancel()

    async def publish_policy(self):
        if self.closed or not self.session or not self.update_supported or not mqtt.is_connected(self.hass):
            return
        message={"schema":1,"key":self.key,"session":self.session,"request":"update_policy","enabled":self.entry.options.get("auto_update_enabled",True),"revision":self.entry.options.get("update_policy_rev",0)}
        await mqtt.async_publish(self.hass,self.base+"/request",json.dumps(message),qos=0,retain=False)

    async def publish(self):
        async with self.lock:
            if self.closed or not self.session or not mqtt.is_connected(self.hass):
                return
            if not self.entities and not self.empty_snapshot_supported:
                return
            if self.seq >= 0xFFFFFFFF:
                self.session = None
                self.request_id = uuid.uuid4().hex
                await self.hello()
                return
            self.seq += 1
            payload = encode_snapshot(self.session, self.key, self.seq, self.entities, self.hass.states.get, presenter=presentation if self.presentation_supported else None, areas=resolve_areas(self.hass, self.entities) if self.areas_supported else None, area_presenter=room_icon if self.area_icons_supported else None)
            await mqtt.async_publish(self.hass, self.base + "/snapshot", payload, qos=0, retain=False)
            self.changed()

    def stop(self):
        self.cancel_backlight()
        self.closed = True
        if self.pending:
            self.pending()
            self.pending = None
        for unsub in reversed(self.unsubs):
            unsub()
        self.unsubs.clear()
        self.listeners.clear()
