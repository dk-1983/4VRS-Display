"""Check known MQTT/loop stack regressions in a compiled classic ESP32 ELF.

This checks the direct compiler-generated frames, not whole-program stack usage.
Run after increasing snapshot/pagination capacity, in addition to OTA validation.
"""
import argparse
from pathlib import Path
import re
import subprocess

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--objdump', type=Path, required=True)
    parser.add_argument('--elf', type=Path, required=True)
    args = parser.parse_args()
    assembly = subprocess.check_output([str(args.objdump), '-Cd', str(args.elf)], text=True)
    for name, limit in [('RackMqtt::tick()', 2048), ('loop()', 2048)]:
        match = re.search(r'^\w+ <'+re.escape(name)+r'>:\s*\n[^\n]*entry\s+a1,\s*(0x[0-9a-f]+|[0-9]+)', assembly, re.M)
        if not match:
            raise SystemExit('Cannot inspect stack frame: '+name)
        size = int(match[1], 0)
        print(f'{name}: direct frame {size} bytes (limit {limit})')
        if size > limit:
            raise SystemExit('Stack regression: '+name)

if __name__ == '__main__': main()
