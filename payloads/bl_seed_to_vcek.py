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
 - with contant label "sev-versioned-chip-endorsement-key" (not null terminated)
 - with an empty context
 - outputs 56 bytes (448 bits)
"""
def kdf(key: bytes) -> bytes:
    output = b''
    output_length = 56

    tail = b'sev-versioned-chip-endorsement-key' + b'\0' + int(output_length*8).to_bytes(4, 'little')
    
    for i in range(1, int(1 + (output_length + 31) / 32)):
        hmac = HMAC(key, hashes.SHA256())
        hmac.update(i.to_bytes(4, 'little') + tail)
        output += hmac.finalize()

    return output[:output_length]

def sha384_hash(data: bytes) -> bytes:
    digest = hashes.Hash(hashes.SHA384())
    digest.update(data)
    return digest.finalize()

def vcek_seed_set_version(seed, version):
    assert 0 <= version
    assert version <= 0xff
    for _ in range(0xff - version):
        seed = sha384_hash(seed)
    return seed

def vcek_seed_dec_version(seed, dec):
    for _ in range(dec):
        seed = sha384_hash(seed)
    return seed

def vcek_seed_move_to_next_version(seed):
    return sha384_hash(b'\0'*8 + seed)

def vcek_seed_get_key(seed):
    return derive_ecdsa_key(kdf(sha384_hash(seed)))

def vcek_get_key_from_tee_seed(tee_seed, tee_ver, snp_ver, ucode_ver):

    tee_seed = vcek_seed_set_version(tee_seed, tee_ver)
    rsv_seed = vcek_seed_move_to_next_version(tee_seed)

    for _ in range(4):
        rsv_seed = vcek_seed_set_version(rsv_seed, 0)
        rsv_seed = vcek_seed_move_to_next_version(rsv_seed)
    snp_seed = rsv_seed

    snp_seed = vcek_seed_set_version(snp_seed, snp_ver)
    ucode_seed = vcek_seed_move_to_next_version(snp_seed)

    ucode_seed = vcek_seed_set_version(ucode_seed, ucode_ver)

    return vcek_seed_get_key(ucode_seed)

if __name__ == '__main__':

    with open(sys.argv[1], 'rb') as f:
        known_bl_seeds = bytes(f.read())

    assert len(known_bl_seeds) == 0x60

    known_bl_seed = known_bl_seeds[:0x30]
    known_tee_seed = known_bl_seeds[0x30:]

    known_bl_seed_version = int(sys.argv[2])

    bl_version = int(sys.argv[3])
    tee_version = int(sys.argv[4])
    snp_version = int(sys.argv[5])
    ucode_version = int(sys.argv[6])

    assert bl_version <= known_bl_seed_version

    if bl_version == known_bl_seed_version:
        tee_seed = known_tee_seed
    else:
        tee_seed = vcek_seed_dec_version(known_bl_seed, known_bl_seed_version - 1 - bl_version)
        tee_seed = vcek_seed_move_to_next_version(tee_seed)
    
    vcek = vcek_get_key_from_tee_seed(tee_seed, tee_version, snp_version, ucode_version)
    priv_nr = vcek.private_numbers()
    pub_nr = vcek.public_key().public_numbers()

    print(f"vcek [bl={hex(bl_version)}, tee={hex(tee_version)}, snp={hex(snp_version)}, ucode={hex(ucode_version)}]:")
    print(f"  d   = {binascii.hexlify(priv_nr.private_value.to_bytes(0x30, 'little')).decode()}")
    print(f"  p_x = {binascii.hexlify(pub_nr.x.to_bytes(0x30, 'little')).decode()}")
    print(f"  p_y = {binascii.hexlify(pub_nr.y.to_bytes(0x30, 'little')).decode()}")

    #print("pem_text pub:")
    #bytestring = b'\4' + pub_nr.x.to_bytes(0x30, 'big') + pub_nr.y.to_bytes(0x30, 'big')
    #for s in range(0, len(bytestring), 15):
    #    e = min(s+15, len(bytestring))
    #    print('          ' + ':'.join(hex(256+v)[3:] for v in bytestring[s:e]))

    with open(sys.argv[7], 'wb') as f:
        f.write(vcek.private_bytes(
            Encoding.PEM,
            PrivateFormat.TraditionalOpenSSL,
            NoEncryption())
        )
 
