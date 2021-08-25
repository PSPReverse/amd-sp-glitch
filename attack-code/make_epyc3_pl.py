#!/usr/bin/env python3
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
import hashlib
import cryptography.hazmat.primitives.serialization as ser
import cryptography.hazmat.primitives.hashes as hashes
import cryptography.hazmat.primitives.asymmetric.rsa as rsa
import cryptography.hazmat.primitives.asymmetric.padding as padding

def printUsage():
    print(f"""
{sys.argv[0]} [output_path]
""")

try:
    output_path = sys.argv[1]
except Exception as e:
    print('Missing some argument!')
    printUsage()
    exit(1)

original_rom = 'roms/original_firmware.bin'
pubkey_offset = 0x66400
pubkey_length = 0x440
firmware_offset = 0x1b6400
firmware_length = 0x15100

pubkey_entry_path = 'OWN_KEY.ENTRY'
privkey_path = 'OWN_KEY.ID'

payload_bin = 'hello-world/hello-world.raw'

def load(fn):
    try:
        with open(fn, 'rb') as f:
            return bytearray(f.read())
    except:
        print(f'Error: Couldn\'t load {fn}')
        raise

rom = load(original_rom)
payload = load(payload_bin)
pubkey_entry = load(pubkey_entry_path)
privkey_bytes = load(privkey_path)
privkey = ser.load_ssh_private_key(privkey_bytes, password=None)

if len(pubkey_entry) != pubkey_length:
    print('pubkey entry has wrong size!')
    exit(1)

if len(payload) > firmware_length:
    print('payload is too large!')
    exit(1)

print('replacing the public key')
rom[pubkey_offset:pubkey_offset + pubkey_length] = pubkey_entry

pubkey_id = pubkey_entry[0x4:0x14]

print('replacing the firmware')

payload_append = 0x10 - (len(payload) % 0x10)
if payload_append != 0x10:
    payload += b'\0' * payload_append

firmware_header_length = 0x100
firmware_signature_length = 0x200

header = rom[firmware_offset:firmware_offset+firmware_header_length]

# set not encrypted
header[0x18:0x1c] = 0x0.to_bytes(4, 'little')
# set iv = 0
header[0x20:0x30] = b'\0'*0x10
# set wrapped key = 0
header[0x80:0x90] = b'\0'*0x10
# set signed size
header[0x14:0x18] = int(len(payload)).to_bytes(4, 'little')
# set pubkey id 
header[0x38:0x48] = pubkey_id
# set packed size
header[0x6c:0x70] = int(len(payload)+firmware_header_length+firmware_signature_length).to_bytes(4, 'little')

# disable hash feature maybe
# header[0x34:0x38] = b'\0'*4
header[0x58:0x5c] = b'\0'*4
header[0xc0:0xf0] = b'\0'*0x30

# set security_version
header[0x4c:0x50] = b'\xff\0\0\0'


rom[firmware_offset:firmware_offset+firmware_header_length] = header

rom[firmware_offset+firmware_header_length:firmware_offset+firmware_header_length+len(payload)] = payload

signature = privkey.sign(
    header + payload,
    padding.PSS(
        mgf=padding.MGF1(hashes.SHA384()),
        salt_length=48,
    ),
    hashes.SHA384()
)

if len(signature) != firmware_signature_length:
    print('signature is too large!')
    exit(1)

rom[firmware_offset+firmware_header_length+len(payload):firmware_offset+firmware_header_length+len(payload)+firmware_signature_length] = signature


print('writing output')
with open(output_path, 'wb') as f:
    f.write(rom)

