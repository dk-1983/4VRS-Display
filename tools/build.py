"""Compile for classic ESP32-WROVER with 4 MB flash and two OTA slots."""
from pathlib import Path
import argparse
import os
import subprocess

ROOT = Path(__file__).resolve().parents[1]
FQBN = 'esp32:esp32:esp32wrover:FlashMode=dio,FlashFreq=40,PartitionScheme=min_spiffs,UploadSpeed=115200'
def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--cli', default='C:/Program Files/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe')
    p.add_argument('--config', type=Path, help='Optional Arduino CLI YAML file')
    a = p.parse_args()
    base = [a.cli] + (['--config-file', str(a.config.resolve())] if a.config else [])
    core = subprocess.check_output(base + ['core', 'list'], text=True)
    if not any(line.split()[:2] == ['esp32:esp32', '3.3.8'] for line in core.splitlines()):
        p.error('Install esp32:esp32@3.3.8 first (see README).')
    if not (ROOT / 'firmware/rack_bootstrap/LocalSecrets.h').exists():
        p.error('Run python tools/init_secrets.py first.')
    build = ROOT / 'build'
    build.mkdir(exist_ok=True)
    temporary = build / 'tmp'
    temporary.mkdir(exist_ok=True)
    environment = os.environ.copy()
    environment.update(TMP=str(temporary), TEMP=str(temporary))
    with (build / 'compile.log').open('w', encoding='utf-8') as log:
        result = subprocess.run(base + ['compile', '--fqbn', FQBN, '--warnings', 'all',
            '--build-path', str(build / 'firmware'), str(ROOT / 'firmware/rack_bootstrap')],
            stdout=log, stderr=subprocess.STDOUT, env=environment)
    print((build / 'compile.log').read_text(encoding='utf-8')[-6000:])
    raise SystemExit(result.returncode)
if __name__ == '__main__':
    main()
