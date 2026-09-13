"""Room semantics and 20-entity boundary, using the real encoder."""
import importlib.util
import json
from pathlib import Path
from types import SimpleNamespace as NS
import unittest
ROOT=Path(__file__).resolve().parents[2]
def load(name):
    spec=importlib.util.spec_from_file_location(name,ROOT/'home_assistant/custom_components/fourvrs_display'/(name+'.py'))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
icons,payload=load('room_icons'),load('payload')
class RoomTests(unittest.TestCase):
    def test_room_names_and_fallback(self):
        expected={'Туалет 1':'toilet','ВАННАЯ':'bathroom','Кухня':'kitchen','Детская спальня':'kids','Спальня':'bedroom','Гостиная':'living','Гараж':'garage','Серверная':'server','Кабинет':'office','Балкон':'balcony','Коридор':'hall','Сад':'garden','Прачечная':'laundry','Кладовая':'storage','Room 17':'room','Kitchen':'kitchen','Guest bedroom':'bedroom','WC':'toilet','Newcastle':'room','':'' or 'room'}
        for name,icon in expected.items():
            with self.subTest(name=name):self.assertEqual(icons.room_icon(name),icon)
        schema=json.loads((ROOT/'protocol/schemas/snapshot-v1.json').read_text())
        self.assertEqual(icons.ICON_NAMES,set(schema['properties']['cards']['items']['properties']['area_icon']['enum']))
    def test_twenty_maximum_cards_and_twenty_first_rejected(self):
        ids=['sensor.'+str(i)+'x'*(89-len(str(i))) for i in range(20)]
        source=NS(state='"'*96,attributes={'friendly_name':'"'*96,'unit_of_measurement':'"'*16})
        wire=payload.encode_snapshot('a'*32,'b'*32,0xffffffff,ids,lambda _:source,presenter=lambda *a:{'icon':'binary_sensor','alert':False},areas={i:('a','"'*48) for i in ids},area_presenter=lambda _:'bathroom')
        self.assertEqual(len(json.loads(wire)['cards']),20)
        self.assertLessEqual(len(wire.encode()),15360)
        self.assertGreater(len(wire.encode()),9216)
        with self.assertRaises(ValueError):payload.validate_selection(ids+['sensor.extra'])
    def test_icon_is_optional_and_derived_before_name_truncation(self):
        args=('a'*32,'b'*32,1,['sensor.a'],lambda _:None)
        area={'sensor.a':('a',' '*49+'Спальня')}
        self.assertNotIn('area_icon',json.loads(payload.encode_snapshot(*args,areas=area))['cards'][0])
        self.assertEqual(json.loads(payload.encode_snapshot(*args,areas=area,area_presenter=icons.room_icon))['cards'][0]['area_icon'],'bedroom')
if __name__=='__main__':unittest.main()
