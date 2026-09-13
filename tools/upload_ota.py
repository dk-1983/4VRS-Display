"""Upload an app binary using pinned espota, then verify the running image over HTTP."""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import logging
import socket
import time
import urllib.request

ROOT = Path(__file__).resolve().parents[1]

def validate_image(path):
    data = path.read_bytes()
    if len(data) < 32 or len(data) > 0x1E0000:
        raise ValueError('Image does not fit the 0x1E0000-byte OTA slot.')
    if data[0] != 0xE9 or int.from_bytes(data[12:14], 'little') != 0:
        raise ValueError('Expected a classic ESP32 application image.')
    # ESP-IDF application descriptor starts after the 24-byte image and 8-byte segment headers.
    if data[32:36] != b'\x32\x54\xcd\xab':
        raise ValueError('Expected an app binary, not a bootloader or merged flash image.')
    return hashlib.md5(data).hexdigest()

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--ip', required=True, help='Device IP or mDNS hostname')
    p.add_argument('--host-ip', required=True, help='PC LAN IPv4 address reachable from ESP32')
    p.add_argument('--host-port', type=int, default=3233)
    p.add_argument('--core', type=Path, required=True, help='Path to esp32/hardware/esp32/3.3.8')
    p.add_argument('--bin', type=Path, default=ROOT / 'build/firmware/rack_bootstrap.ino.bin')
    p.add_argument('--access', type=Path, default=ROOT / 'private/access.json')
    a = p.parse_args()
    try:
        digest = validate_image(a.bin)
        ip = socket.gethostbyname(a.ip)
        password = json.loads(a.access.read_text(encoding='utf-8'))['ota_password']
        if len(password) < 16: raise ValueError('Missing or too short OTA password.')
        if a.core.name != '3.3.8': raise ValueError('Use the pinned ESP32 core 3.3.8.')
        spec = importlib.util.spec_from_file_location('espota_pinned', a.core / 'tools/espota.py')
        ota = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(ota)
    except (OSError, ValueError, KeyError) as e:
        p.error(str(e))
    # Avoid proxies for this local management connection. Credentials never enter argv/logs.
    http = urllib.request.build_opener(urllib.request.ProxyHandler({}))
    def health():
        with http.open('http://' + ip + '/health', timeout=10) as response:
            return json.load(response)
    before = None
    for attempt in range(3):
        try:
            before = health()
            break
        except (OSError, ValueError) as error:
            if attempt == 2:
                p.error('Device /health is unavailable: ' + str(error))
            time.sleep(1)
    if not before.get('ota') or not str(before.get('hostname', '')).startswith('4vrs-rack-'):
        p.error('Target is not a ready 4VRS bootstrap device.')
    ota.PROGRESS = True
    ota.TIMEOUT = 3
    logging.basicConfig(level=logging.WARNING)
    result = ota.serve(ip, a.host_ip, 3232, a.host_port, password, False, str(a.bin.resolve()), ota.FLASH)
    if result != 0:
        raise SystemExit('OTA transfer failed. Running firmware has not been verified.')
    # espota can return zero even for an ambiguous final reply; verify actual image and slot.
    deadline = time.monotonic() + 60
    while time.monotonic() < deadline:
        time.sleep(2)
        try:
            after = health()
            if (after.get('hostname') == before['hostname'] and after.get('sketch_md5') == digest
                    and after.get('partition') != before.get('partition') and after.get('ota')):
                print('Verified: image MD5, device identity, changed OTA slot, and OTA service after reboot.')
                return
        except (OSError, ValueError):
            pass
    raise SystemExit('Transfer ended, but boot verification failed. Check IP and Serial; do not assume success.')

if __name__ == '__main__':
    main()
