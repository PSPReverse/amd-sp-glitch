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
from binascii import hexlify
from IPython import embed

from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend

def swap_bytes(data,len):
    data_int = int.from_bytes(data, 'little')
    return (data_int).to_bytes(len, 'big')

def decrypt_ecb(data,ikek):
    cipher_ecb = Cipher(algorithms.AES(ikek), modes.ECB(), backend=backend)
    ecb_decryptor = cipher_ecb.decryptor()
    return ecb_decryptor.update(data) + ecb_decryptor.finalize()

def decrypt_cbd(data,iv,key):
    cipher_cbc = Cipher(algorithms.AES(key), modes.CBC(iv), backend=backend)
    cbc_decryptor= cipher_cbc.decryptor()
    return cbc_decryptor.update(data) + cbc_decryptor.finalize()

backend = default_backend()

try:
    with open(sys.argv[1], 'rb') as f:
        ikek = f.read()
    with open(sys.argv[2], 'rb') as f:
        entry = bytes(f.read())
    output_file = sys.argv[3]
except:
    print(f"USAGE: {sys.argv[0]} <ikek binary file> <entry file> <output file>")
    raise

entry_key_enc = entry[0x80:0x90]
entry_size = int.from_bytes(entry[0x14:0x18],'little')
entry_iv = entry[0x20:0x30]

entry_key = decrypt_ecb(entry_key_enc, ikek)

content = decrypt_cbd(entry[0x100:0x100+entry_size], entry_iv, entry_key)

with open(output_file, 'wb') as f:
    f.write(content)

