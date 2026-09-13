"""Verify semantic classification, safety states, and the expanded wire budget."""
import importlib.util
import json
from pathlib import Path
from types import SimpleNamespace
import unittest
ROOT=Path(__file__).resolve().parents[2]
def module(name):
    spec=importlib.util.spec_from_file_location(name, ROOT/'home_assistant/custom_components/fourvrs_display'/f'{name}.py')
    m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
p=module('presentation');payload=module('payload')
class PresentationTests(unittest.TestCase):
    def test_all_documented_sensor_classes_and_domains_fit_protocol(self):
        schema=json.loads((ROOT/'protocol/schemas/snapshot-v1.json').read_text())
        icons=schema['properties']['cards']['items']['properties']['icon']['enum']
        for domain, classes in [('sensor',p.SENSOR_ICONS),('binary_sensor',p.BINARY_ICONS)]:
            for cls in classes:
                with self.subTest(domain=domain, cls=cls):
                    result=p.presentation(domain+'.test', {'device_class':cls}, 'on')
                    self.assertIn(result['icon'], icons)
                    self.assertLessEqual(len(result['icon'].encode()),16)
        for domain in p.DOMAIN_ICONS:
            self.assertIn(p.presentation(domain+'.test',{},'idle')['icon'],icons)
        self.assertGreater(len(p.SENSOR_ICONS),50)
        self.assertEqual(len(p.BINARY_ICONS),28)
    def test_moisture_and_battery_semantics_depend_on_domain(self):
        for state in ['off','unknown','unavailable']:
            self.assertFalse(p.presentation('binary_sensor.x',{'device_class':'moisture'},state)['alert'])
        self.assertEqual(p.presentation('sensor.x',{'device_class':'moisture'},'50')['icon'],'humidity')
        self.assertEqual(p.presentation('binary_sensor.x',{'device_class':'moisture'},'on'),{'icon':'leak','alert':True})
        self.assertTrue(p.presentation('binary_sensor.x',{'device_class':'battery'},'on')['alert'])
        self.assertFalse(p.presentation('sensor.x',{'device_class':'battery'},'10')['alert'])
    def test_authoritative_class_overrides_name_and_unknowns_fall_back(self):
        self.assertEqual(p.presentation('sensor.temperature',{'device_class':'humidity','friendly_name':'Temperature'},'50')['icon'],'humidity')
        self.assertEqual(p.presentation('sensor.x',{'device_class':'future_class'},'v')['icon'],'sensor')
        self.assertEqual(p.presentation('custom_domain.x',{},'v')['icon'],'text')
        self.assertEqual(p.presentation('switch.x',{},'off')['icon'],'switch')
        self.assertEqual(p.presentation('switch.x',{'friendly_name':'lighting Tasmota2'},'off')['icon'],'light')
    def test_twelve_maximum_cards_with_metadata_fit(self):
        ids=['sensor.'+str(i)+'x'*87 for i in range(12)]
        source=SimpleNamespace(state='"'*96,attributes={'friendly_name':'"'*96,'unit_of_measurement':'"'*16,'device_class':'temperature'})
        wire=payload.encode_snapshot('a'*32,'b'*32,1,ids,lambda _:source,presenter=p.presentation)
        self.assertLessEqual(len(wire.encode()),8192)
        self.assertEqual(len(json.loads(wire)['cards']),12)
        self.assertTrue(all(c['icon']=='temperature' for c in json.loads(wire)['cards']))
    def test_old_sender_shape_stays_supported(self):
        wire=payload.encode_snapshot('a'*32,'b'*32,1,['sensor.x'],lambda _:None)
        self.assertNotIn('icon',json.loads(wire)['cards'][0])
if __name__=='__main__':unittest.main()
