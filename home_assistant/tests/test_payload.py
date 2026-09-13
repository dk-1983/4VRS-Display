"""Protocol boundary tests using the real HA encoder, without a HA installation."""
import importlib.util
import json
from pathlib import Path
from types import SimpleNamespace
import unittest

ROOT = Path(__file__).resolve().parents[2]
source = ROOT / 'home_assistant/custom_components/fourvrs_display/payload.py'
spec = importlib.util.spec_from_file_location('display_payload', source)
payload = importlib.util.module_from_spec(spec)
spec.loader.exec_module(payload)


class PayloadTests(unittest.TestCase):
    def encode(self, entities, lookup=lambda _: None):
        return json.loads(payload.encode_snapshot('a'*32, 'b'*32, 1, entities, lookup))

    def test_unavailable_is_not_off(self):
        result = self.encode(['fan.server', 'light.room'], lambda e: SimpleNamespace(state='off', attributes={}) if e.startswith('light') else None)
        self.assertEqual([c['state'] for c in result['cards']], ['unavailable', 'off'])

    def test_unicode_byte_limits_do_not_split_codepoints(self):
        source = SimpleNamespace(state='23.4', attributes={'friendly_name': 'Вентиляция 🌬'*100, 'unit_of_measurement': '°C'})
        result = self.encode(['sensor.room'], lambda _: source)
        self.assertLessEqual(len(result['cards'][0]['name'].encode()), 96)
        self.assertNotIn('\ufffd', result['cards'][0]['name'])
        self.assertEqual(result['cards'][0]['unit'], '°C')

    def test_controls_are_removed_and_unknown_type_falls_back(self):
        source = SimpleNamespace(state='\x00idle\n', attributes={'friendly_name': 'A\rB'})
        result = self.encode(['climate.room'], lambda _: source)
        self.assertEqual(result['cards'][0]['kind'], 'text')
        self.assertEqual(result['cards'][0]['state'], 'idle')

    def test_selection_limits_and_topic_injection(self):
        for entities in ([], ['sensor.a']*2, ['sensor.a']*4, ['sensor.a/#'], ['sensor.a\n'], 'sensor.a'):
            with self.assertRaises(ValueError):
                self.encode(entities)

    def test_invalid_session_and_sequence(self):
        for seq in (0, -1, True, 2**32):
            with self.assertRaises(ValueError):
                payload.encode_snapshot('a'*32, 'b'*32, seq, ['sensor.a'], lambda _: None)
        with self.assertRaises(ValueError):
            payload.encode_snapshot('retained-old-session', 'b'*32, 1, ['sensor.a'], lambda _: None)

    def test_handshake_requires_device_and_challenge(self):
        caps = {'schema': 1, 'device_id': 'test', 'session': 'a'*32, 'max_cards': 3, 'request_id': 'challenge'}
        self.assertEqual(payload.decode_capabilities(json.dumps(caps), 'test', 'challenge'), caps)
        for device, challenge in [('other', 'challenge'), ('test', 'old')]:
            with self.assertRaises(ValueError):
                payload.decode_capabilities(json.dumps(caps), device, challenge)


if __name__ == '__main__':
    unittest.main()
