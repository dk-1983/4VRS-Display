"""One display session: state subscriptions, bounded coalescing, heartbeat and ACK."""
import asyncio
from datetime import timedelta
import json
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
        self.presentation_supported = False
        self.areas_supported = False
        self.area_icons_supported = False
        self.seq = self.ack_seq = 0
        self.last_ack = self.last_hello = 0.0
        self.waiting_since = time.monotonic()
        self.request_id = uuid.uuid4().hex
        self.status = "waiting"
        self.firmware = None
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
                self.session = caps["session"]
                self.seq = self.ack_seq = 0
                self.last_ack = 0.0
                self.waiting_since = time.monotonic()
                self.status = "sending"
            self.presentation_supported = caps.get("presentation_v") == 1
            self.areas_supported = caps.get("area_v") == 1 and type(caps.get("max_payload")) is int and caps["max_payload"] >= 15360
            self.area_icons_supported = self.areas_supported and caps.get("area_icon_v") == 1
            self.firmware = caps.get("firmware")
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
        if self.closed:
            return
        if not self.session or self.status in ("offline", "waiting"):
            await self.hello()
        else:
            if time.monotonic() - (self.last_ack or self.waiting_since) > 90:
                self.status = "stale"
                self.changed()
            await self.publish()

    async def publish(self):
        async with self.lock:
            if self.closed or not self.session or not mqtt.is_connected(self.hass):
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
        self.closed = True
        if self.pending:
            self.pending()
            self.pending = None
        for unsub in reversed(self.unsubs):
            unsub()
        self.unsubs.clear()
        self.listeners.clear()
