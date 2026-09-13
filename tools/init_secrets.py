"""Create local credentials once; never print passwords or overwrite existing ones."""
from pathlib import Path
import json
import secrets

root = Path(__file__).resolve().parents[1]
header = root / 'firmware/rack_bootstrap/LocalSecrets.h'
private = root / 'private/access.json'
if header.exists() or private.exists():
    raise SystemExit('Credentials already exist. Kept unchanged.')
data = {'setup_password': secrets.token_urlsafe(18), 'ota_password': secrets.token_urlsafe(24)}
private.parent.mkdir(exist_ok=True)
private.write_text(json.dumps(data, indent=2) + '\n', encoding='utf-8')
header.write_text('#pragma once\n#define SETUP_PASSWORD ' + json.dumps(data['setup_password']) +
                  '\n#define OTA_PASSWORD ' + json.dumps(data['ota_password']) + '\n', encoding='utf-8')
print('Created private/access.json and LocalSecrets.h; passwords are not printed.')
