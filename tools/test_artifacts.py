"""Host checks for OTA image rejection and one-time credentials. Does not emulate ESP32."""
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest

TOOLS = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('upload', TOOLS / 'upload_ota.py')
upload = importlib.util.module_from_spec(spec)
spec.loader.exec_module(upload)

class Artifacts(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.image = self.root / 'app.bin'
    def tearDown(self):
        self.temp.cleanup()
    def candidate(self, chip=0):
        data = bytearray(128)
        data[0] = 0xe9
        data[12:14] = chip.to_bytes(2, 'little')
        data[32:36] = b'\x32\x54\xcd\xab'
        return data
    def test_accept_classic_esp32_app_header(self):
        self.image.write_bytes(self.candidate())
        self.assertEqual(len(upload.validate_image(self.image)), 32)
    def test_reject_other_chip(self):
        self.image.write_bytes(self.candidate(chip=9))
        with self.assertRaises(ValueError): upload.validate_image(self.image)
    def test_reject_oversize(self):
        data = self.candidate() + bytes(0x1e0000)
        self.image.write_bytes(data)
        with self.assertRaises(ValueError): upload.validate_image(self.image)
    def test_reject_bootloader(self):
        data = self.candidate(); data[32:36] = bytes(4)
        self.image.write_bytes(data)
        with self.assertRaises(ValueError): upload.validate_image(self.image)
    def test_reject_empty_and_merged(self):
        for data in (b'', bytes(128), b'\xff' * 4096 + self.candidate()):
            self.image.write_bytes(data)
            with self.assertRaises(ValueError): upload.validate_image(self.image)
    def test_credentials_are_not_overwritten_or_printed(self):
        (self.root / 'tools').mkdir()
        (self.root / 'firmware/rack_bootstrap').mkdir(parents=True)
        script = self.root / 'tools/init_secrets.py'
        shutil.copyfile(TOOLS / 'init_secrets.py', script)
        run = subprocess.run([sys.executable, str(script)], capture_output=True, text=True)
        self.assertEqual(run.returncode, 0)
        path = self.root / 'private/access.json'
        original = path.read_bytes()
        data = json.loads(original)
        for value in data.values():
            self.assertNotIn(value, run.stdout + run.stderr)
        second = subprocess.run([sys.executable, str(script)], capture_output=True, text=True)
        self.assertNotEqual(second.returncode, 0)
        self.assertEqual(original, path.read_bytes())
    def test_embedded_bitmap_layout(self):
        data = (TOOLS.parent / 'assets/demo-240x320.bmp').read_bytes()
        self.assertEqual(data[:2], b'BM')
        self.assertEqual(struct.unpack_from('<I', data, 10)[0], 66)
        self.assertEqual(struct.unpack_from('<ii', data, 18), (240, -320))
        self.assertEqual(struct.unpack_from('<III', data, 54), (0xf800, 0x7e0, 0x1f))
        self.assertEqual(len(data), 66 + 240 * 320 * 2)
        self.assertEqual(data[66:], (TOOLS.parent / 'assets/demo-240x320.rgb565').read_bytes())

if __name__ == '__main__': unittest.main()
