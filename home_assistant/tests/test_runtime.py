"""Exercise the real session state machine with a controlled MQTT/HA boundary."""
import importlib
import json
from pathlib import Path
import sys
import time
import types
import unittest

ROOT = Path(__file__).resolve().parents[2]
package = types.ModuleType('display_test_pkg')
package.__path__ = [str(ROOT / 'home_assistant/custom_components/fourvrs_display')]
sys.modules['display_test_pkg'] = package


class RuntimeTests(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        self.originals = {name: sys.modules.get(name) for name in ['homeassistant', 'homeassistant.components', 'homeassistant.components.mqtt', 'homeassistant.core', 'homeassistant.helpers', 'homeassistant.helpers.event']}
        for name in self.originals:
            sys.modules[name] = types.ModuleType(name)
        self.sent, self.subs = [], {}
        self.closed = 0
        self.delayed = []
        mqtt = sys.modules['homeassistant.components.mqtt']
        mqtt.is_connected = lambda hass: True

        async def publish(hass, topic, body, **kwargs):
            self.sent.append((topic, json.loads(body), kwargs))

        async def subscribe(hass, topic, callback, **kwargs):
            self.subs[topic] = callback
            return lambda: self.subs.pop(topic, None)

        mqtt.async_publish, mqtt.async_subscribe = publish, subscribe
        sys.modules['homeassistant.core'].callback = lambda f: f
        events = sys.modules['homeassistant.helpers.event']

        def track(*args):
            return lambda: setattr(self, 'closed', self.closed + 1)

        def later(hass, delay, callback):
            self.delayed.append(callback)
            return lambda: self.delayed.remove(callback)

        events.async_track_state_change_event = events.async_track_time_interval = track
        events.async_call_later = later
        sys.modules.pop('display_test_pkg.runtime', None)
        module = importlib.import_module('display_test_pkg.runtime')
        entry = types.SimpleNamespace(data={'device_id': 'test', 'pairing_key': 'b'*32, 'entities': ['fan.server']}, options={})
        hass = types.SimpleNamespace(states=types.SimpleNamespace(get=lambda _: types.SimpleNamespace(state='on', attributes={'friendly_name': 'Вентилятор'})))
        self.runtime = module.DisplayRuntime(hass, entry)
        await self.runtime.start()

    async def asyncTearDown(self):
        self.runtime.stop()
        for name, value in self.originals.items():
            if value is None:
                sys.modules.pop(name, None)
            else:
                sys.modules[name] = value

    async def connect(self, session='a'*32):
        caps = {'schema': 1, 'device_id': 'test', 'session': session, 'max_cards': 3, 'request_id': self.runtime.request_id}
        await self.runtime.capabilities(types.SimpleNamespace(payload=json.dumps(caps)))

    async def test_requires_handshake_and_sends_nonretained_snapshot(self):
        await self.runtime.publish()
        self.assertTrue(all(topic.endswith('/request') for topic, _, _ in self.sent))
        await self.connect()
        topic, body, flags = self.sent[-1]
        self.assertTrue(topic.endswith('/snapshot'))
        self.assertFalse(flags['retain'])
        self.assertEqual(body['cards'][0]['state'], 'on')

    async def test_ack_cannot_come_from_old_session_or_future_sequence(self):
        await self.connect()
        for session, seq in [('c'*32, 1), ('a'*32, 99)]:
            self.runtime.ack(types.SimpleNamespace(payload=json.dumps({'schema': 1, 'session': session, 'seq': seq, 'status': 'accepted'})))
        self.assertEqual(self.runtime.ack_seq, 0)
        self.runtime.ack(types.SimpleNamespace(payload=json.dumps({'schema': 1, 'session': 'a'*32, 'seq': 1, 'status': 'accepted'})))
        self.assertEqual(self.runtime.status, 'accepted')
        await self.connect('d'*32)
        self.assertEqual(self.runtime.ack_seq, 0)
        self.assertEqual(self.runtime.seq, 1)

    async def test_high_rate_updates_coalesce_and_stop_cleans_up(self):
        await self.connect()
        for _ in range(100):
            self.runtime.state_changed(None)
        self.assertEqual(len(self.delayed), 1)
        self.runtime.stop()
        self.assertFalse(self.subs)
        self.assertFalse(self.delayed)
        self.assertEqual(self.closed, 2)
        before = len(self.sent)
        await self.runtime.publish()
        self.assertEqual(before, len(self.sent))

    async def test_missing_ack_is_stale_even_without_any_previous_ack(self):
        await self.connect()
        self.runtime.waiting_since = time.monotonic() - 100
        await self.runtime.heartbeat(None)
        self.assertEqual(self.runtime.status, 'stale')


if __name__ == '__main__':
    unittest.main()
