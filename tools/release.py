"""Prepare a signed stable firmware feed from a public build (cryptography required)."""
import argparse
import hashlib
import json
from pathlib import Path
import re
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec

PROFILE = 'nadim-v5-ili9341-4m-v1'
REPO = 'dk-1983/4VRS-Display'

def validate_build(build):
    meta = json.loads((build/'build-kind.json').read_text())
    version = meta['version']
    if not meta.get('public') or meta.get('migration') or meta.get('test_fail_boot'):
        raise ValueError('Private migration/test build cannot be released')
    if not re.fullmatch(r'[0-9]{1,5}\.[0-9]{1,5}\.[0-9]{1,5}', version):
        raise ValueError('Expected stable numeric version')
    data = (build/'rack_bootstrap.ino.bin').read_bytes()
    if not 128 <= len(data) <= 0x1e0000 or data[0] != 0xe9 or data[12:14] != b'\0\0' or data[32:36] != b'\x32\x54\xcd\xab':
        raise ValueError('Expected compatible ESP32 app image within OTA slot')
    return version, data

def envelope(payload, key):
    raw = json.dumps(payload, separators=(',', ':'), sort_keys=True)
    signature = key.sign(raw.encode(), ec.ECDSA(hashes.SHA256()))
    key.public_key().verify(signature, raw.encode(), ec.ECDSA(hashes.SHA256()))
    return {'payload': raw, 'signature': signature.hex()}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build', type=Path, required=True)
    p.add_argument('--key', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--private-access', type=Path, help='Local credential scan; values never printed')
    a=p.parse_args();version,data=validate_build(a.build)
    if a.private_access:
        credentials=json.loads(a.private_access.read_text(encoding='utf-8'))
        for name,value in credentials.items():
            if name!='setup_password' and isinstance(value,str) and len(value)>=8 and value.encode() in data:
                raise ValueError('Local credential found in public image; release rejected')
    key=serialization.load_pem_private_key(a.key.read_bytes(),password=None)
    if not isinstance(key,ec.EllipticCurvePrivateKey) or not isinstance(key.curve,ec.SECP256R1):
        raise ValueError('Expected ECDSA P-256 signing key')
    name=f'4vrs-display-{version}.bin'
    payload={'schema':1,'channel':'stable','profile':PROFILE,'version':version,
             'url':f'https://github.com/{REPO}/releases/download/firmware-v{version}/{name}',
             'size':len(data),'sha256':hashlib.sha256(data).hexdigest()}
    signed=envelope(payload,key)
    a.out.mkdir(parents=True,exist_ok=True)
    (a.out/name).write_bytes(data)
    (a.out/'stable.json').write_text(json.dumps(signed,indent=2)+'\n',encoding='utf-8')
    (a.out/'SHA256SUMS').write_text(f"{payload['sha256']}  {name}\n",encoding='utf-8')
    print('Prepared signed public firmware',version,'bytes',len(data))

if __name__=='__main__':main()
