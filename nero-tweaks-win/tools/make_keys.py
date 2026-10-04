#!/usr/bin/env python3
"""Run ONCE. Creates your private signing key (keep it secret!) and pubkey.inc for the app.
Usage: python3 make_keys.py <private_key_output.pem> <pubkey.inc output>"""
import sys
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives import serialization
priv, inc = sys.argv[1], sys.argv[2]
k = ec.generate_private_key(ec.SECP256R1())
open(priv, "wb").write(k.private_bytes(serialization.Encoding.PEM, serialization.PrivateFormat.PKCS8, serialization.NoEncryption()))
n = k.public_key().public_numbers()
b = n.x.to_bytes(32, "big") + n.y.to_bytes(32, "big")
open(inc, "w").write("static const unsigned char PUBKEY[64] = {" + ",".join("0x%02x" % c for c in b) + "};\n")
print("wrote", priv, "and", inc)
