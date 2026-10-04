#!/usr/bin/env python3
"""Convert nero_private_key.pem into nero_private_key.bin (used by NeroCodeMaker.exe).
Usage: python3 pem_to_bin.py nero_private_key.pem nero_private_key.bin"""
import sys, struct
from cryptography.hazmat.primitives import serialization
k = serialization.load_pem_private_key(open(sys.argv[1], "rb").read(), None)
pn = k.private_numbers(); pub = pn.public_numbers
blob = struct.pack("<II", 0x32534345, 32) + pub.x.to_bytes(32, "big") + pub.y.to_bytes(32, "big") + pn.private_value.to_bytes(32, "big")
open(sys.argv[2], "wb").write(blob); print("wrote", sys.argv[2], len(blob), "bytes")
