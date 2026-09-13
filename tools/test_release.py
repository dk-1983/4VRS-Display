"""Signing and private-artifact publication boundaries (no device required)."""
import json
import tempfile
import unittest
from pathlib import Path
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.exceptions import InvalidSignature
from release import envelope, validate_build

class ReleaseTests(unittest.TestCase):
    def test_signed_payload_tampering(self):
        key=ec.generate_private_key(ec.SECP256R1())
        signed=envelope({"version":"0.4.0","sha256":"a"*64},key)
        signature=bytes.fromhex(signed["signature"])
        key.public_key().verify(signature,signed["payload"].encode(),ec.ECDSA(hashes.SHA256()))
        with self.assertRaises(InvalidSignature):
            key.public_key().verify(signature,signed["payload"].replace("0.4.0","9.0.0").encode(),ec.ECDSA(hashes.SHA256()))

    def test_private_and_oversized_images_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            p=Path(folder)
            for flag in ["migration","test_fail_boot","test_feed"]:
                (p/"build-kind.json").write_text(json.dumps({"version":"0.4.0","public":True,flag:True}))
                with self.assertRaises(ValueError):validate_build(p)
            (p/"build-kind.json").write_text(json.dumps({"version":"0.4.0","public":True}))
            (p/"rack_bootstrap.ino.bin").write_bytes(bytes(0x1e0001))
            with self.assertRaises(ValueError):validate_build(p)

if __name__=="__main__":unittest.main()
