"""Map HA domains/device classes to compact, bounded display icon families.

Reference: home-assistant.io/integrations/{sensor,binary_sensor}/ (2026-09-13).
Unrecognized classes/domains always retain their original text value.
"""
import re

SENSOR_ICONS = {}
for icon, classes in {
    'temperature': 'temperature temperature_delta',
    'humidity': 'humidity absolute_humidity moisture',
    'power': 'apparent_power current energy energy_distance energy_storage frequency power power_factor reactive_energy reactive_power voltage',
    'air': 'aqi carbon_dioxide carbon_monoxide nitrogen_dioxide nitrogen_monoxide nitrous_oxide ozone pm1 pm25 pm4 pm10 radon sulphur_dioxide volatile_organic_compounds volatile_organic_compounds_parts',
    'ruler': 'area distance speed weight',
    'pressure': 'atmospheric_pressure pressure',
    'battery': 'battery',
    'sensor': 'blood_glucose_concentration conductivity ph',
    'network': 'data_rate signal_strength',
    'storage': 'data_size',
    'clock': 'date duration timestamp uptime',
    'text': 'enum',
    'gas': 'gas',
    'sun': 'illuminance irradiance',
    'money': 'monetary',
    'water': 'precipitation precipitation_intensity volume volume_flow_rate volume_storage water',
    'sound': 'sound_pressure',
    'wind': 'wind_direction wind_speed',
}.items():
    SENSOR_ICONS.update(dict.fromkeys(classes.split(), icon))

BINARY_ICONS = {
    'battery': 'battery', 'battery_charging': 'battery', 'carbon_monoxide': 'gas',
    'cold': 'temperature', 'connectivity': 'network', 'door': 'door',
    'garage_door': 'door', 'gas': 'gas', 'heat': 'temperature', 'light': 'sun',
    'lock': 'lock', 'moisture': 'leak', 'motion': 'person', 'moving': 'motion',
    'occupancy': 'person', 'opening': 'door', 'plug': 'plug', 'power': 'power',
    'presence': 'person', 'problem': 'warning', 'running': 'motion',
    'safety': 'warning', 'smoke': 'smoke', 'sound': 'sound', 'tamper': 'warning',
    'update': 'update', 'vibration': 'motion', 'window': 'window',
}
ALARM_CLASSES = {'battery', 'carbon_monoxide', 'gas', 'heat', 'moisture', 'problem', 'safety', 'smoke', 'tamper'}
DOMAIN_ICONS = {
    'air_quality': 'air', 'alarm_control_panel': 'shield', 'automation': 'settings',
    'binary_sensor': 'binary_sensor', 'button': 'button', 'calendar': 'clock',
    'camera': 'camera', 'climate': 'temperature', 'conversation': 'text',
    'counter': 'sensor', 'cover': 'window', 'date': 'clock', 'datetime': 'clock',
    'device_tracker': 'person', 'event': 'event', 'fan': 'fan', 'group': 'group',
    'humidifier': 'humidity', 'image': 'image', 'image_processing': 'image',
    'input_boolean': 'switch', 'input_button': 'button', 'input_datetime': 'clock',
    'input_number': 'sensor', 'input_select': 'list', 'input_text': 'text',
    'lawn_mower': 'robot', 'light': 'light', 'lock': 'lock', 'media_player': 'media',
    'notify': 'event', 'number': 'sensor', 'person': 'person', 'remote': 'remote',
    'scene': 'image', 'schedule': 'clock', 'script': 'settings', 'select': 'list',
    'sensor': 'sensor', 'siren': 'sound', 'stt': 'sound', 'sun': 'sun',
    'switch': 'switch', 'text': 'text', 'time': 'clock', 'timer': 'clock',
    'todo': 'list', 'tts': 'sound', 'update': 'update', 'vacuum': 'robot',
    'valve': 'valve', 'wake_word': 'sound', 'water_heater': 'temperature',
    'weather': 'weather', 'zone': 'location',
}
MDI_ICONS = {'mdi:lightbulb': 'light', 'mdi:lightbulb-outline': 'light',
             'mdi:ceiling-light': 'light', 'mdi:outdoor-lamp': 'light',
             'mdi:fan': 'fan', 'mdi:water-percent': 'humidity',
             'mdi:thermometer': 'temperature'}
ICON_NAMES = sorted(set(SENSOR_ICONS.values()) | set(BINARY_ICONS.values()) | set(DOMAIN_ICONS.values()) | {'text'})

def presentation(entity_id, attributes, state):
    domain = entity_id.split('.', 1)[0]
    device_class = str(attributes.get('device_class') or '')
    classes = SENSOR_ICONS if domain == 'sensor' else BINARY_ICONS if domain == 'binary_sensor' else {}
    icon = classes.get(device_class)
    if icon is None:
        icon = MDI_ICONS.get(attributes.get('icon'))
    if icon is None and domain == 'switch' and device_class == 'outlet':
        icon = 'plug'
    if icon is None and domain == 'switch' and re.search(r'\b(light|lighting)\b', str(attributes.get('friendly_name', '')), re.I):
        icon = 'light'
    if icon is None:
        icon = DOMAIN_ICONS.get(domain, 'text')
    alarm = (domain == 'binary_sensor' and device_class in ALARM_CLASSES and state == 'on') or (domain == 'alarm_control_panel' and state == 'triggered')
    return {'icon': icon, 'alert': alarm}
