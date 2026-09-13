"""Build public ESP32 firmware, or an explicitly private credential-migration bridge."""
from pathlib import Path
import argparse
import os
import re
import subprocess

ROOT=Path(__file__).resolve().parents[1]
FQBN='esp32:esp32:esp32wrover:FlashMode=dio,FlashFreq=40,PartitionScheme=min_spiffs,UploadSpeed=115200'
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--cli',default='C:/Program Files/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe')
    p.add_argument('--config',type=Path)
    p.add_argument('--version',default='0.4.4')
    p.add_argument('--output',type=Path,default=ROOT/'build/firmware')
    p.add_argument('--migrate-credentials',action='store_true',help='PRIVATE one-time migration; never publish this binary')
    p.add_argument('--test-fail-boot',action='store_true',help='PRIVATE rollback test; never publish this binary')
    p.add_argument('--test-feed',action='store_true',help='PRIVATE: use testing.json, never publish this image')
    a=p.parse_args()
    if not re.fullmatch(r'[0-9]{1,5}\.[0-9]{1,5}\.[0-9]{1,5}',a.version):p.error('Use numeric major.minor.patch')
    if a.migrate_credentials and not (ROOT/'firmware/rack_bootstrap/LocalSecrets.h').exists():p.error('Migration requires existing private LocalSecrets.h')
    base=[a.cli]+(['--config-file',str(a.config.resolve())] if a.config else [])
    core=subprocess.check_output(base+['core','list'],text=True)
    if not any(line.split()[:2]==['esp32:esp32','3.3.8'] for line in core.splitlines()):p.error('Install esp32:esp32@3.3.8')
    build=a.output.resolve();build.mkdir(parents=True,exist_ok=True)
    tmp=ROOT/'build/tmp';tmp.mkdir(parents=True,exist_ok=True)
    flags='-DFOURVRS_VERSION='+a.version
    if a.migrate_credentials:flags+=' -DFOURVRS_MIGRATE_CREDENTIALS=1'
    if a.test_feed:flags+=' -DFOURVRS_TEST_FEED=1'
    if a.test_fail_boot:flags+=' -DFOURVRS_TEST_FAIL_BOOT=1'
    cmd=base+['compile','--fqbn',FQBN,'--warnings','all','--build-property','compiler.cpp.extra_flags='+flags,'--build-path',str(build),str(ROOT/'firmware/rack_bootstrap')]
    log_path=build.parent/(build.name+'.compile.log')
    with log_path.open('w',encoding='utf-8') as log:
        result=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,env={**os.environ,'TMP':str(tmp),'TEMP':str(tmp)})
    build.mkdir(parents=True,exist_ok=True)
    output=log_path.read_text(encoding='utf-8')
    (build/'compile.log').write_text(output,encoding='utf-8')
    print(output[-5000:])
    if result.returncode==0:
        import json
        (build/'build-kind.json').write_text(json.dumps({'version':a.version,'public':not(a.migrate_credentials or a.test_fail_boot or a.test_feed),'migration':a.migrate_credentials,'test_fail_boot':a.test_fail_boot,'test_feed':a.test_feed}))
    raise SystemExit(result.returncode)
if __name__=='__main__':main()
