#!/usr/bin/env python3
"""Make a license code for ONE customer PC.
Usage: python3 keygen.py <private_key.pem> NERO-XXXX-XXXX-XXXX-XXXX
The customer copies their PC ID from the 'Get Pro' page in Nero Tweaks and sends it to you."""
import sys, base64, re
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric.utils import decode_dss_signature
from cryptography.hazmat.primitives import hashes, serialization
pem, pc = sys.argv[1], sys.argv[2].strip().upper()
if not re.fullmatch(r"NERO-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{4}", pc):
    sys.exit("That does not look like a Nero PC ID (expected NERO-XXXX-XXXX-XXXX-XXXX).")
key = serialization.load_pem_private_key(open(pem, "rb").read(), None)
der = key.sign(("NERO1|" + pc).encode(), ec.ECDSA(hashes.SHA256()))
r, s = decode_dss_signature(der)
code = base64.b32encode(r.to_bytes(32, "big") + s.to_bytes(32, "big")).decode().rstrip("=")
print("-".join(code[i:i + 8] for i in range(0, len(code), 8)))
