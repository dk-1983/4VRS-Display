"""Area inheritance, stable grouping and worst-case wire budget."""
import importlib.util
import json
from pathlib import Path
from types import ModuleType, SimpleNamespace as NS
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
def load(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / 'home_assistant/custom_components/fourvrs_display' / (name + '.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module
areas, payload = load('areas'), load('payload')

class AreaTests(unittest.TestCase):
    def test_entity_override_device_inheritance_and_unassigned(self):
        entries = {'sensor.own': NS(area_id='a', device_id='dev'), 'sensor.device': NS(area_id=None, device_id='dev'), 'sensor.none': NS(area_id=None, device_id=None), 'sensor.deleted': NS(area_id='deleted', device_id='dev')}
        devices = {'dev': NS(area_id='b')}
        rooms = {'a': NS(id='a', name='Балкон'), 'b': NS(id='b', name='Туалет')}
        modules = {name: ModuleType(name) for name in ['homeassistant', 'homeassistant.helpers', 'homeassistant.helpers.area_registry', 'homeassistant.helpers.entity_registry', 'homeassistant.helpers.device_registry']}
        ar, er, dr = (modules['homeassistant.helpers.'+name] for name in ['area_registry','entity_registry','device_registry'])
        ar.async_get = lambda _: NS(async_get_area=rooms.get)
        er.async_get = lambda _: NS(async_get=entries.get)
        dr.async_get = lambda _: NS(async_get=devices.get)
        with patch.dict('sys.modules', modules):
            result = areas.resolve_areas(None, list(entries)+['sensor.missing'])
            self.assertEqual(result, {'sensor.own': ('a','Балкон'), 'sensor.device': ('b','Туалет'), 'sensor.none': ('',''), 'sensor.deleted': ('',''), 'sensor.missing': ('','')})
            # Current HA resolves child-device inheritance through its public helper.
            er.async_get_effective_area_id = lambda hass, entry: 'a'
            self.assertEqual(areas.resolve_areas(None,['sensor.device'])['sensor.device'], ('a','Балкон'))
            rooms['a'].name = 'Гараж'
            self.assertEqual(areas.resolve_areas(None,['sensor.device'])['sensor.device'][1], 'Гараж')

    def test_group_sort_and_original_selection_are_stable(self):
        ids = ['sensor.z', 'sensor.none', 'sensor.a2', 'sensor.a1', 'sensor.z2']
        original = ids.copy()
        assigned = {ids[0]:('z','Туалет'), ids[2]:('a','Балкон'), ids[3]:('a','Балкон'), ids[4]:('z','Туалет')}
        wire = payload.encode_snapshot('a'*32,'b'*32,1,ids,lambda _:None,areas=assigned)
        cards = json.loads(wire)['cards']
        self.assertEqual([c['id'] for c in cards], ['sensor.a2','sensor.a1','sensor.z','sensor.z2','sensor.none'])
        self.assertEqual([c['area'] for c in cards], ['Балкон','Балкон','Туалет','Туалет',''])
        self.assertEqual(ids,original)
        self.assertTrue(all(c['state']=='unavailable' for c in cards))

    def test_twelve_maximum_escaped_cards_fit_with_areas(self):
        ids = ['sensor.'+str(i)+'x'*(89-len(str(i))) for i in range(12)]
        state = NS(state='"'*96,attributes={'friendly_name':'"'*96,'unit_of_measurement':'"'*16})
        wire = payload.encode_snapshot('a'*32,'b'*32,0xffffffff,ids,lambda _:state,presenter=lambda *args:{'icon':'binary_sensor','alert':False},areas={i:('a','"'*48) for i in ids})
        self.assertLessEqual(len(wire.encode()),payload.MAX_PAYLOAD)
        self.assertGreater(len(wire.encode()),8192)
        self.assertEqual(len(json.loads(wire)['cards']),12)

    def test_utf8_area_label_and_legacy_omission(self):
        args = ('a'*32,'b'*32,1,['sensor.a'],lambda _:None)
        card = json.loads(payload.encode_snapshot(*args,areas={'sensor.a':('a','Комната 🌞'*30)}))['cards'][0]
        self.assertLessEqual(len(card['area'].encode()),48)
        self.assertNotIn('\ufffd',card['area'])
        self.assertNotIn('area',json.loads(payload.encode_snapshot(*args))['cards'][0])

if __name__ == '__main__': unittest.main()
