# Copyright (C) 2021 Niklas Jacob
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
import cryptography.hazmat.primitives.serialization as ser
import cryptography.hazmat.primitives.asymmetric.rsa as rsa
import hashlib
import struct

def generate_privkey(size=2048):
    return rsa.generate_private_key(key_size=size, public_exponent=0x10001)

def make_pubkey_entry(public_key, size=2048):
    public_numbers = public_key.public_numbers()
    e = public_numbers.e
    n = public_numbers.n

    version = 0x1.to_bytes(4, 'little')
    key_id = 0xdeadbeef.to_bytes(4, 'big') * 4
    cert_id = key_id # self-signed
    key_usage = 0x0.to_bytes(4, 'little') # amd public key
    reserved = 0x0.to_bytes(4, 'big') * 4
    pubex_size = int(size).to_bytes(4, 'little')
    modul_size = int(size).to_bytes(4, 'little')

    header = version \
        + key_id \
        + cert_id \
        + key_usage \
        + reserved \
        + pubex_size \
        + modul_size

    assert len(header) == 0x40

    return header + e.to_bytes((size>>3), 'little') + n.to_bytes((size>>3), 'little')

def serialize_privkey(private_key):
    return private_key.private_bytes(encoding=ser.Encoding.PEM, format=ser.PrivateFormat.OpenSSH, encryption_algorithm=ser.NoEncryption())

def get_pubkey_entry_hash(public_key_entry, size=2048):
    return hashlib.sha256(public_key_entry[:(0x40+2*(size>>3))]).digest()

def print_hash(h):
    hex_bytes = [hex(i+0x100)[3:] for i in struct.unpack('<' + 'B'*32, h)]
    hex_words = []
    for i in range(8):
        hex_words.append(''.join(hex_bytes[i*4:(i+1)*4]))
    print(' '.join(hex_words))

if __name__ == '__main__':

    size = 4096

    while True:
        private_key = generate_privkey(size)
        public_key = private_key.public_key()
        public_key_entry = make_pubkey_entry(public_key, size)

        print('generated public key hash:')
        print_hash(get_pubkey_entry_hash(public_key_entry, size))

        ans = input('is hash okay? [y/N]: ')
        if ans and ans in "yY":
            break

    key_name = sys.argv[1]

    entry_file = f'{key_name}.ENTRY'
    privkey_file = f'{key_name}.ID'

    with open(entry_file, 'wb') as f:
        f.write(public_key_entry)
    print(f'Wrote public key entry to: {entry_file}')

    with open(privkey_file, 'wb') as f:
        f.write(serialize_privkey(private_key))
    print(f'Wrote private key to: {privkey_file}')

