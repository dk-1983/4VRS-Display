"""Configure a display over authenticated HTTP; credentials never enter argv/output."""
import argparse
import json
from pathlib import Path
import re
import urllib.request

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ip', required=True)
    parser.add_argument('--settings', type=Path, default=ROOT / 'private/mqtt.json')
    parser.add_argument('--access', type=Path, default=ROOT / 'private/access.json')
    parser.add_argument('--export-ha', type=Path, help='Optional private file for device ID and pairing key')
    args = parser.parse_args()
    if not re.fullmatch(r'[a-zA-Z0-9.-]+', args.ip):
        parser.error('Expected a local IP or hostname')
    try:
        values = json.loads(args.settings.read_text(encoding='utf-8-sig'))
        access = json.loads(args.access.read_text(encoding='utf-8-sig')) if args.access.exists() else {}
        password = access.get('web_password', 'admin')
        config = {k: values[k] for k in ('host', 'port', 'username', 'password', 'ca') if k in values}
        config.setdefault('host', '')
        config.setdefault('port', 1883)
        config.setdefault('username', '')
        config.update(enabled=values.get('enabled', True), tls=values.get('tls', False))
        if 'ca_file' in values:
            config['ca'] = (args.settings.parent / values['ca_file']).read_text(encoding='ascii')
        base = 'http://' + args.ip
        manager = urllib.request.HTTPPasswordMgrWithDefaultRealm()
        manager.add_password(None, base, access.get('web_username', 'admin'), password)
        http = urllib.request.build_opener(urllib.request.ProxyHandler({}), urllib.request.HTTPDigestAuthHandler(manager))
        with http.open(base + '/mqtt', timeout=10) as reply:
            page = reply.read().decode()
        token = re.search(r"let token='([0-9a-f]{32})'", page)
        if not token:
            raise ValueError('Not a compatible MQTT settings page')
        config['token'] = token.group(1)
        req = urllib.request.Request(base + '/mqtt/config', data=json.dumps(config).encode(), headers={'Content-Type': 'application/json'})
        with http.open(req, timeout=10) as reply:
            if not json.load(reply).get('saved'):
                raise ValueError('Configuration was not saved')
        if args.export_ha:
            target = args.export_ha.resolve()
            if not target.is_relative_to((ROOT / 'private').resolve()):
                raise ValueError('Export pairing credentials only inside project private/')
            with http.open(base + '/mqtt/config', timeout=10) as reply:
                current = json.load(reply)
            target.write_text(json.dumps({k: current[k] for k in ('device_id', 'pairing_key')}, indent=2) + '\n', encoding='utf-8')
        print('Settings saved. Check /mqtt for connection status; no credentials printed.')
    except (OSError, ValueError, KeyError):
        # Never interpolate an HTTP body, payload or credentials into a failure log.
        parser.exit(1, 'MQTT setup failed. Check the settings file, device address and local access credentials.\n')


if __name__ == '__main__':
    main()
