#!/usr/bin/env python
# Copyright (C) 2021 Niklas Jacob, Robert Buhren
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

import sys
import binascii
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.hmac import HMAC
from cryptography.hazmat.primitives.serialization import Encoding, NoEncryption, PrivateFormat

"""
The id is dervied as follows:
 - interpret the 256 secret fuse bits as as big-endian number d
 - on the SecP256k1 curve calculate (x,y) = d*Q
 - the id is the concatenation of x and y as bit-endian numbers

In other words, the secret fuses are the private key and the id
is the public key of an SecP256k1 key-pair.
"""
def get_id(secret_fuses: bytes) -> bytes:

    assert(len(secret_fuses) == 0x20)

    d = int.from_bytes(secret_fuses, 'big')
    curve = ec.SECP256K1()

    priv_key = ec.derive_private_key(d, curve)
    pub_key = priv_key.public_key()
    pub_nums = pub_key.public_numbers()

    x = pub_nums.x.to_bytes(0x20, 'big')
    y = pub_nums.y.to_bytes(0x20, 'big')

    return x + y

"""
Attention: NON SECURE IMPLEMENTATION, DON'T GENERATE CONFIDENTIAL KEYS WITH THIS METHOD!

ECDSA key generation using extra random bits
(as described in B.4.1 of FIPS PUB 186-4 [1])

Note: we are not interpreting the incoming bits as a big endian
      number, since the sev implementation doesn't either.

using Nist P384 (as described in [1]) aka SecP384r1

[1]: https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.186-4.pdf"
"""
def derive_ecdsa_key(random_bits : bytes) -> ec.EllipticCurvePrivateKey:

    # order of the group on p384
    n = 39402006196394479212279040100143613805079739270465446667946905279627659399113263569398956308152294913554433653942643

    c = int.from_bytes(random_bits, 'little')
    assert(len(random_bits)*8 == n.bit_length() + 64)

    # this step is probably vulnerable to side-channel attacks
    d = (c % (n - 1)) + 1

    curve = ec.SECP384R1()
    return ec.derive_private_key(d, curve)

"""
Attention: NON SECURE IMPLEMENTATION, DON'T GENERATE CONFIDENTIAL KEYS WITH THIS METHOD!

NISTs SP800-108 KDF
 - in counter mode
 - using HMAC-SHA256 as PRF
 - counters and legth represented as 4-byte little endian numbers (hazmat uses big endian *facepalm*)
 - with contant label "sev-chip-endorsement-key" (not null terminated)
 - with an empty context
 - outputs 56 bytes (448 bits)
"""
def kdf(key: bytes) -> bytes:
    output = b''
    output_length = 56

    tail = b'sev-chip-endorsement-key' + b'\0' + int(output_length*8).to_bytes(4, 'little')
    
    for i in range(1, int(1 + (output_length + 31) / 32)):
        hmac = HMAC(key, hashes.SHA256())
        hmac.update(i.to_bytes(4, 'little') + tail)
        output += hmac.finalize()

    return output[:output_length]

"""
The CEK is derived as follows:
 - hash the secret fuses using sha256
 - use the kdf to generate random bits
 - generate an ECDSA key on the P384 curve using these random bits
"""
def get_cek(secret_fuses : bytes) -> ec.EllipticCurvePrivateKey:

    # sha256(secret fuses)
    digest = hashes.Hash(hashes.SHA256())
    digest.update(secret_fuses)
    digest = digest.finalize()

    # generate random bits
    random_bits = kdf(digest)

    return derive_ecdsa_key(random_bits)

if __name__ == '__main__':

    with open(sys.argv[1], 'rb') as f:
        secret_fuses = bytearray(f.read())

    print("secret fuses:")
    print(f"  *   = {binascii.hexlify(secret_fuses).decode()}")

    cek_id = get_id(secret_fuses)

    print("cek:")
    print(f"  id  = {binascii.hexlify(cek_id).decode()}")

    cek = get_cek(secret_fuses)

    print(f"  d   = {binascii.hexlify(cek.private_numbers().private_value.to_bytes(0x30, 'little')).decode()}")
    print(f"  p_x = {binascii.hexlify(cek.public_key().public_numbers().x.to_bytes(0x30, 'little')).decode()}")
    print(f"  p_y = {binascii.hexlify(cek.public_key().public_numbers().y.to_bytes(0x30, 'little')).decode()}")

    with open(sys.argv[2], 'wb') as f:
        f.write(cek.private_bytes(
            Encoding.PEM,
            PrivateFormat.TraditionalOpenSSL,
            NoEncryption())
        )
 
